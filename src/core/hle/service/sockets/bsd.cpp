// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <array>
#include <cstdlib>
#include <cstring>
#include <map>
#include <mutex>
#include <optional>
#include <memory>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include <fmt/ranges.h>

#ifdef __linux__
#include <fcntl.h>
#include <sys/eventfd.h>
#include <unistd.h>
#endif
#ifndef _WIN32
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#endif

#include "common/hex_util.h"
#include "common/logging.h"
#include "common/socket_types.h"
#include "core/core.h"
#include "core/hle/kernel/k_thread.h"
#include "core/hle/service/ipc_helpers.h"
#include "core/hle/service/sockets/bsd.h"
#include "core/hle/service/sockets/online_redirect.h"
#include "core/hle/service/sockets/sockets_translate.h"
#include "core/internal_network/network.h"
#include "core/internal_network/socket_proxy.h"
#include "core/internal_network/sockets.h"
#include "network/network.h"
#include <common/settings.h>

namespace Service::Sockets {

namespace {

constexpr u32 TcpLevel = 6;   // IPPROTO_TCP
constexpr u32 TcpNoDelay = 1; // TCP_NODELAY

u64 OptionKey(u32 level, OptName optname) {
    return (u64{level} << 32) | static_cast<u32>(optname);
}

// SendMMsg/RecvMMsg (29/30): the SDK implements sendmsg/recvmsg with them (Raiders' gRPC uses
// both). Messages travel in one in/out buffer, serialized by nn::socket::detail (sdk): a version
// byte (8), then per message
//   u32 namelen, name[namelen], u32 iovlen, {u64 len, data[len]} x iovlen,
//   u32 controllen, control[controllen], u32 flags, u32 msg_len.
// For RecvMMsg the data areas are empty space to fill. The SDK reads the lengths back to find
// each field, so the layout stays fixed: received data, the sender's address and msg_len are
// written into it in place.
struct MMsgSlot {
    size_t name_offset = 0;
    u32 name_len = 0;
    std::vector<std::pair<size_t, u64>> iovs; // data offset, length
    size_t control_offset = 0;
    u32 control_len = 0;
    size_t flags_offset = 0; // msg_flags, then msg_len
};

bool ParseMMsgBuffer(std::span<const u8> buffer, u32 vlen, std::vector<MMsgSlot>& out) {
    size_t pos = 1; // version byte
    const auto take = [&](size_t n) {
        if (buffer.size() < pos || buffer.size() - pos < n) {
            return false;
        }
        pos += n;
        return true;
    };
    const auto read_u32 = [&](u32& v) {
        if (!take(sizeof(u32))) {
            return false;
        }
        std::memcpy(&v, buffer.data() + pos - sizeof(u32), sizeof(u32));
        return true;
    };
    for (u32 i = 0; i < vlen; i++) {
        MMsgSlot slot;
        u32 iov_count = 0;
        if (!read_u32(slot.name_len)) {
            return false;
        }
        slot.name_offset = pos;
        if (!take(slot.name_len) || !read_u32(iov_count)) {
            return false;
        }
        for (u32 j = 0; j < iov_count; j++) {
            u64 len = 0;
            if (!take(sizeof(u64))) {
                return false;
            }
            std::memcpy(&len, buffer.data() + pos - sizeof(u64), sizeof(u64));
            slot.iovs.emplace_back(pos, len);
            if (!take(static_cast<size_t>(len))) {
                return false;
            }
        }
        if (!read_u32(slot.control_len)) {
            return false;
        }
        slot.control_offset = pos;
        if (!take(slot.control_len)) {
            return false;
        }
        slot.flags_offset = pos;
        if (!take(2 * sizeof(u32))) {
            return false;
        }
        out.push_back(std::move(slot));
    }
    return true;
}

std::vector<u8> ReadBufferB(HLERequestContext& ctx) {
    if (ctx.BufferDescriptorB().empty()) {
        return {};
    }
    const auto& desc = ctx.BufferDescriptorB()[0];
    std::vector<u8> buffer(desc.Size());
    ctx.GetMemory().ReadBlock(desc.Address(), buffer.data(), buffer.size());
    return buffer;
}

#if defined(__linux__) || defined(_WIN32)
// A guest eventfd (bsd EventFd) backed by a host eventfd, so that the guest's Read/Write of 8 bytes
// and Poll behave like the real thing. gRPC wakes its poller through one; with the old stub (fd 0,
// reads returning 0) Raiders' NPLN client never started a connection. Reads never block: a blocking
// read would hold the bsd service thread that the matching Write needs.
// Windows has no eventfd: the counter is kept here instead, and a UDP socket on 127.0.0.1 connected
// to itself holds one datagram while the counter is nonzero, so that Poll sees it as readable.
class EventFdSocket final : public Network::SocketBase {
public:
#ifdef _WIN32
    EventFdSocket(u64 initval, bool semaphore_) : semaphore{semaphore_} {
        fd = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        auto* const sa = reinterpret_cast<sockaddr*>(&addr);
        int len = sizeof(addr);
        if (fd == INVALID_SOCKET || ::bind(fd, sa, len) != 0 || ::getsockname(fd, sa, &len) != 0 ||
            ::connect(fd, sa, len) != 0) {
            Close();
            return;
        }
        // The drain in Recv waits for a datagram that is still on its way, but never for long
        const DWORD timeout_ms = 100;
        ::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout_ms),
                     sizeof(timeout_ms));
        Add(initval);
    }
#else
    EventFdSocket(u64 initval, bool semaphore)
        : SocketBase{::eventfd(static_cast<unsigned int>(initval),
                               EFD_NONBLOCK | EFD_CLOEXEC | (semaphore ? EFD_SEMAPHORE : 0))} {}
#endif

    ~EventFdSocket() override {
        Close();
    }

    Network::Errno Initialize(Network::Domain, Network::Type, Network::Protocol) override {
        return Network::Errno::SUCCESS;
    }

    Network::Errno Close() override {
        if (fd != INVALID_SOCKET) {
#ifdef _WIN32
            ::closesocket(fd);
#else
            ::close(fd);
#endif
            fd = INVALID_SOCKET;
        }
        return Network::Errno::SUCCESS;
    }

    std::pair<AcceptResult, Network::Errno> Accept() override {
        return {AcceptResult{}, Network::Errno::INVAL};
    }

    Network::Errno Connect(Network::SockAddrIn) override {
        return Network::Errno::INVAL;
    }

    std::pair<Network::SockAddrIn, Network::Errno> GetPeerName() override {
        return {Network::SockAddrIn{}, Network::Errno::NOTCONN};
    }

    std::pair<Network::SockAddrIn, Network::Errno> GetSockName() override {
        return {Network::SockAddrIn{}, Network::Errno::INVAL};
    }

    Network::Errno Bind(Network::SockAddrIn) override {
        return Network::Errno::INVAL;
    }

    Network::Errno Listen(s32) override {
        return Network::Errno::INVAL;
    }

    Network::Errno Shutdown(Network::ShutdownHow) override {
        return Network::Errno::SUCCESS;
    }

    std::pair<s32, Network::Errno> Recv(int, std::span<u8> message) override {
        if (message.size() < sizeof(u64)) {
            return {-1, Network::Errno::INVAL};
        }
#ifdef _WIN32
        std::scoped_lock lock{mutex};
        if (count == 0) {
            return {-1, Network::Errno::AGAIN};
        }
        const u64 value = semaphore ? 1 : count;
        count -= value;
        if (count == 0) {
            char byte;
            ::recv(fd, &byte, sizeof(byte), 0);
        }
        std::memcpy(message.data(), &value, sizeof(value));
        return {static_cast<s32>(sizeof(value)), Network::Errno::SUCCESS};
#else
        const ssize_t n = ::read(fd, message.data(), sizeof(u64));
        if (n < 0) {
            return {-1, errno == EAGAIN ? Network::Errno::AGAIN : Network::Errno::OTHER};
        }
        return {static_cast<s32>(n), Network::Errno::SUCCESS};
#endif
    }

    std::pair<s32, Network::Errno> RecvFrom(int flags, std::span<u8> message,
                                            Network::SockAddrIn*) override {
        return Recv(flags, message);
    }

    std::pair<s32, Network::Errno> Send(std::span<const u8> message, int) override {
        if (message.size() < sizeof(u64)) {
            return {-1, Network::Errno::INVAL};
        }
#ifdef _WIN32
        u64 value;
        std::memcpy(&value, message.data(), sizeof(value));
        if (value == ~u64{0}) {
            return {-1, Network::Errno::INVAL};
        }
        std::scoped_lock lock{mutex};
        if (value > ~u64{0} - 1 - count) {
            return {-1, Network::Errno::AGAIN};
        }
        Add(value);
        return {static_cast<s32>(sizeof(value)), Network::Errno::SUCCESS};
#else
        const ssize_t n = ::write(fd, message.data(), sizeof(u64));
        if (n < 0) {
            return {-1, errno == EAGAIN ? Network::Errno::AGAIN : Network::Errno::OTHER};
        }
        return {static_cast<s32>(n), Network::Errno::SUCCESS};
#endif
    }

    std::pair<s32, Network::Errno> SendTo(u32 flags, std::span<const u8> message,
                                          const Network::SockAddrIn*) override {
        return Send(message, static_cast<int>(flags));
    }

    Network::Errno SetLinger(bool, u32) override {
        return Network::Errno::SUCCESS;
    }
    Network::Errno SetReuseAddr(bool) override {
        return Network::Errno::SUCCESS;
    }
    Network::Errno SetKeepAlive(bool) override {
        return Network::Errno::SUCCESS;
    }
    Network::Errno SetBroadcast(bool) override {
        return Network::Errno::SUCCESS;
    }
    Network::Errno SetSndBuf(u32) override {
        return Network::Errno::SUCCESS;
    }
    Network::Errno SetRcvBuf(u32) override {
        return Network::Errno::SUCCESS;
    }
    Network::Errno SetSndTimeo(u32) override {
        return Network::Errno::SUCCESS;
    }
    Network::Errno SetRcvTimeo(u32) override {
        return Network::Errno::SUCCESS;
    }
    Network::Errno SetNonBlock(bool) override {
        return Network::Errno::SUCCESS; // always non-blocking, see above
    }

    std::pair<Network::Errno, Network::Errno> GetPendingError() override {
        return {Network::Errno::SUCCESS, Network::Errno::SUCCESS};
    }

    bool IsOpened() const override {
        return fd != INVALID_SOCKET;
    }

    void HandleProxyPacket(const Network::ProxyPacket&) override {}

#ifdef _WIN32
private:
    // Callers hold the mutex (or are the constructor)
    void Add(u64 value) {
        if (count == 0 && value != 0) {
            const char byte = 0;
            ::send(fd, &byte, sizeof(byte), 0);
        }
        count += value;
    }

    std::mutex mutex;
    u64 count = 0;
    bool semaphore;
#endif
};
#endif

#ifdef __linux__
// A guest raw ICMP socket (SOCK_RAW, IPPROTO_ICMP) as a host ping socket (SOCK_DGRAM,
// IPPROTO_ICMP), which Linux allows without privileges (net.ipv4.ping_group_range); a host raw
// socket needs root. Raiders measures latency with ICMP echo through one (once a second during
// matchmaking). A raw socket receives the IP header and sees the echo id it sent, so receives
// get a synthetic IPv4 header and the guest's id (with the checksum fixed) back.
class IcmpPingSocket final : public Network::SocketBase {
public:
    IcmpPingSocket() : SocketBase{::socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, IPPROTO_ICMP)} {}

    ~IcmpPingSocket() override {
        Close();
    }

    Network::Errno Initialize(Network::Domain, Network::Type, Network::Protocol) override {
        return Network::Errno::SUCCESS;
    }

    Network::Errno Close() override {
        if (fd != INVALID_SOCKET) {
            ::close(fd);
            fd = INVALID_SOCKET;
        }
        return Network::Errno::SUCCESS;
    }

    std::pair<AcceptResult, Network::Errno> Accept() override {
        return {AcceptResult{}, Network::Errno::INVAL};
    }

    Network::Errno Connect(Network::SockAddrIn addr_in) override {
        peer = addr_in;
        return Network::Errno::SUCCESS;
    }

    std::pair<Network::SockAddrIn, Network::Errno> GetPeerName() override {
        return {peer.value_or(Network::SockAddrIn{}), peer ? Network::Errno::SUCCESS
                                                           : Network::Errno::NOTCONN};
    }

    std::pair<Network::SockAddrIn, Network::Errno> GetSockName() override {
        return {Network::SockAddrIn{.family = Network::Domain::INET}, Network::Errno::SUCCESS};
    }

    Network::Errno Bind(Network::SockAddrIn) override {
        return Network::Errno::SUCCESS;
    }

    Network::Errno Listen(s32) override {
        return Network::Errno::INVAL;
    }

    Network::Errno Shutdown(Network::ShutdownHow) override {
        return Network::Errno::SUCCESS;
    }

    std::pair<s32, Network::Errno> Recv(int flags, std::span<u8> message) override {
        return RecvFrom(flags, message, nullptr);
    }

    std::pair<s32, Network::Errno> RecvFrom(int, std::span<u8> message,
                                            Network::SockAddrIn* addr) override {
        std::array<u8, 2048> icmp;
        sockaddr_in from{};
        socklen_t from_len = sizeof(from);
        const ssize_t n = ::recvfrom(fd, icmp.data(), icmp.size(), 0,
                                     reinterpret_cast<sockaddr*>(&from), &from_len);
        if (n < 0) {
            return {-1, HostErrno(errno)};
        }
        if (n >= 8 && has_echo_id) {
            icmp[4] = static_cast<u8>(echo_id >> 8);
            icmp[5] = static_cast<u8>(echo_id);
            icmp[2] = icmp[3] = 0;
            const u16 sum = Checksum(icmp.data(), static_cast<size_t>(n));
            icmp[2] = static_cast<u8>(sum >> 8);
            icmp[3] = static_cast<u8>(sum);
        }
        std::array<u8, 20> ip{0x45, 0, 0, 0, 0, 0, 0, 0, 64, 1};
        const size_t total = ip.size() + static_cast<size_t>(n);
        ip[2] = static_cast<u8>(total >> 8);
        ip[3] = static_cast<u8>(total);
        std::memcpy(ip.data() + 12, &from.sin_addr, 4);
        const u16 ip_sum = Checksum(ip.data(), ip.size());
        ip[10] = static_cast<u8>(ip_sum >> 8);
        ip[11] = static_cast<u8>(ip_sum);
        const size_t head = std::min(ip.size(), message.size());
        std::memcpy(message.data(), ip.data(), head);
        const size_t body = std::min(static_cast<size_t>(n), message.size() - head);
        std::memcpy(message.data() + head, icmp.data(), body);
        if (addr != nullptr) {
            addr->family = Network::Domain::INET;
            std::memcpy(addr->ip.data(), &from.sin_addr, 4);
            addr->portno = 0;
        }
        return {static_cast<s32>(head + body), Network::Errno::SUCCESS};
    }

    std::pair<s32, Network::Errno> Send(std::span<const u8> message, int flags) override {
        if (!peer) {
            return {-1, Network::Errno::NOTCONN};
        }
        return SendTo(static_cast<u32>(flags), message, &*peer);
    }

    std::pair<s32, Network::Errno> SendTo(u32, std::span<const u8> message,
                                          const Network::SockAddrIn* addr) override {
        if (addr == nullptr) {
            return Send(message, 0);
        }
        if (message.size() >= 8) {
            echo_id = static_cast<u16>(message[4] << 8 | message[5]);
            has_echo_id = true;
        }
        sockaddr_in to{};
        to.sin_family = AF_INET;
        std::memcpy(&to.sin_addr, addr->ip.data(), 4);
        const ssize_t n = ::sendto(fd, message.data(), message.size(), 0,
                                   reinterpret_cast<const sockaddr*>(&to), sizeof(to));
        const int error = n < 0 ? errno : 0;
        if (addr->ip != last_destination) {
            last_destination = addr->ip;
            LOG_INFO(Service, "ICMP echo to {} ({} bytes): {}",
                     Network::IPv4AddressToString(addr->ip), message.size(),
                     n < 0 ? std::strerror(error) : "sent");
        }
        if (n < 0) {
            return {-1, HostErrno(error)};
        }
        return {static_cast<s32>(n), Network::Errno::SUCCESS};
    }

    Network::Errno SetLinger(bool, u32) override {
        return Network::Errno::SUCCESS;
    }
    Network::Errno SetReuseAddr(bool) override {
        return Network::Errno::SUCCESS;
    }
    Network::Errno SetKeepAlive(bool) override {
        return Network::Errno::SUCCESS;
    }
    Network::Errno SetBroadcast(bool) override {
        return Network::Errno::SUCCESS;
    }
    Network::Errno SetSndBuf(u32) override {
        return Network::Errno::SUCCESS;
    }
    Network::Errno SetRcvBuf(u32) override {
        return Network::Errno::SUCCESS;
    }
    Network::Errno SetSndTimeo(u32) override {
        return Network::Errno::SUCCESS;
    }
    Network::Errno SetRcvTimeo(u32) override {
        return Network::Errno::SUCCESS;
    }

    Network::Errno SetNonBlock(bool enable) override {
        const int flags = ::fcntl(fd, F_GETFL);
        ::fcntl(fd, F_SETFL, enable ? flags | O_NONBLOCK : flags & ~O_NONBLOCK);
        return Network::Errno::SUCCESS;
    }

    std::pair<Network::Errno, Network::Errno> GetPendingError() override {
        return {Network::Errno::SUCCESS, Network::Errno::SUCCESS};
    }

    bool IsOpened() const override {
        return fd != INVALID_SOCKET;
    }

    void HandleProxyPacket(const Network::ProxyPacket&) override {}

private:
    static Network::Errno HostErrno(int error) {
        switch (error) {
        case EAGAIN:
            return Network::Errno::AGAIN;
        case EINVAL:
            return Network::Errno::INVAL;
        case ENETUNREACH:
            return Network::Errno::NETUNREACH;
        case EHOSTUNREACH:
            return Network::Errno::HOSTUNREACH;
        case EMSGSIZE:
            return Network::Errno::MSGSIZE;
        case ECONNREFUSED:
            return Network::Errno::CONNREFUSED;
        case ENETDOWN:
            return Network::Errno::NETDOWN;
        default:
            return Network::Errno::INVAL;
        }
    }

    static u16 Checksum(const u8* data, size_t size) {
        u32 sum = 0;
        for (size_t i = 0; i + 1 < size; i += 2) {
            sum += static_cast<u32>(data[i] << 8 | data[i + 1]);
        }
        if (size % 2 != 0) {
            sum += static_cast<u32>(data[size - 1] << 8);
        }
        while (sum >> 16) {
            sum = (sum & 0xffff) + (sum >> 16);
        }
        return static_cast<u16>(~sum);
    }

    std::optional<Network::SockAddrIn> peer;
    Network::IPv4Address last_destination{};
    u16 echo_id = 0;
    bool has_echo_id = false;
};
#endif


bool IsConnectionBased(Type type) {
    switch (type) {
    case Type::STREAM:
        return true;
    case Type::DGRAM:
        return false;
    default:
        UNIMPLEMENTED_MSG("Unimplemented type={}", type);
        return false;
    }
}

template <typename T>
T GetValue(std::span<const u8> buffer) {
    T t{};
    std::memcpy(&t, buffer.data(), (std::min)(sizeof(T), buffer.size()));
    return t;
}

template <typename T>
void PutValue(std::span<u8> buffer, const T& t) {
    std::memcpy(buffer.data(), &t, (std::min)(sizeof(T), buffer.size()));
}

} // Anonymous namespace

void BSD_USA::PollWork::Execute(BSD_USA* bsd) {
    std::tie(ret, bsd_errno) = bsd->PollImpl(write_buffer, read_buffer, nfds, timeout);
}

void BSD_USA::PollWork::Response(HLERequestContext& ctx) {
    if (write_buffer.size() > 0) {
        ctx.WriteBuffer(write_buffer);
    }

    IPC::ResponseBuilder rb{ctx, 4};
    rb.Push(ResultSuccess);
    rb.Push<s32>(ret);
    rb.PushEnum(bsd_errno);
}

void BSD_USA::AcceptWork::Execute(BSD_USA* bsd) {
    std::tie(ret, bsd_errno) = bsd->AcceptImpl(fd, write_buffer);
}

void BSD_USA::AcceptWork::Response(HLERequestContext& ctx) {
    if (write_buffer.size() > 0) {
        ctx.WriteBuffer(write_buffer);
    }

    IPC::ResponseBuilder rb{ctx, 5};
    rb.Push(ResultSuccess);
    rb.Push<s32>(ret);
    rb.PushEnum(bsd_errno);
    rb.Push<u32>(static_cast<u32>(write_buffer.size()));
}

void BSD_USA::ConnectWork::Execute(BSD_USA* bsd) {
    bsd_errno = bsd->ConnectImpl(fd, addr);
}

void BSD_USA::ConnectWork::Response(HLERequestContext& ctx) {
    IPC::ResponseBuilder rb{ctx, 4};
    rb.Push(ResultSuccess);
    rb.Push<s32>(bsd_errno == Errno::SUCCESS ? 0 : -1);
    rb.PushEnum(bsd_errno);
}

void BSD_USA::RecvWork::Execute(BSD_USA* bsd) {
    std::tie(ret, bsd_errno) = bsd->RecvImpl(fd, flags, message);
}

void BSD_USA::RecvWork::Response(HLERequestContext& ctx) {
    ctx.WriteBuffer(message);

    IPC::ResponseBuilder rb{ctx, 4};
    rb.Push(ResultSuccess);
    rb.Push<s32>(ret);
    rb.PushEnum(bsd_errno);
}

void BSD_USA::RecvFromWork::Execute(BSD_USA* bsd) {
    std::tie(ret, bsd_errno) = bsd->RecvFromImpl(fd, flags, message, addr);
}

void BSD_USA::RecvFromWork::Response(HLERequestContext& ctx) {
    ctx.WriteBuffer(message, 0);
    if (!addr.empty()) {
        ctx.WriteBuffer(addr, 1);
    }

    IPC::ResponseBuilder rb{ctx, 5};
    rb.Push(ResultSuccess);
    rb.Push<s32>(ret);
    rb.PushEnum(bsd_errno);
    rb.Push<u32>(static_cast<u32>(addr.size()));
}

void BSD_USA::SendWork::Execute(BSD_USA* bsd) {
    std::tie(ret, bsd_errno) = bsd->SendImpl(fd, flags, message);
}

void BSD_USA::SendWork::Response(HLERequestContext& ctx) {
    IPC::ResponseBuilder rb{ctx, 4};
    rb.Push(ResultSuccess);
    rb.Push<s32>(ret);
    rb.PushEnum(bsd_errno);
}

void BSD_USA::SendToWork::Execute(BSD_USA* bsd) {
    std::tie(ret, bsd_errno) = bsd->SendToImpl(fd, flags, message, addr);
}

void BSD_USA::SendToWork::Response(HLERequestContext& ctx) {
    IPC::ResponseBuilder rb{ctx, 4};
    rb.Push(ResultSuccess);
    rb.Push<s32>(ret);
    rb.PushEnum(bsd_errno);
}

void BSD_USA::RegisterClient(HLERequestContext& ctx) {
    LOG_WARNING(Service, "(STUBBED) called");

    IPC::ResponseBuilder rb{ctx, 3};

    rb.Push(ResultSuccess);
    rb.Push<s32>(0); // bsd errno
}

// [18.0.0+] Cmd36: on Switch 2 the SDK registers with this instead of RegisterClient*. It takes the
// PID, a u32 LibraryConfigData version (upper u16 is 1 on Switch 2) and a u64 pid_reserved, and
// returns 4 bytes like RegisterClient. Raiders aborts when the reply lacks them (going online).
void BSD_USA::RegisterClientWithVersion(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const u32 version = rp.Pop<u32>();
    LOG_WARNING(Service, "(STUBBED) called, version={:#x}", version);

    IPC::ResponseBuilder rb{ctx, 3};

    rb.Push(ResultSuccess);
    rb.Push<s32>(0); // bsd errno
}

void BSD_USA::SendMMsg(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const s32 fd = rp.Pop<s32>();
    const u32 vlen = rp.Pop<u32>();
    const u32 flags = rp.Pop<u32>();

    std::vector<u8> buffer = ReadBufferB(ctx);
    std::vector<MMsgSlot> slots;
    s32 ret = -1;
    Errno bsd_errno = Errno::INVAL;
    if (ParseMMsgBuffer(buffer, vlen, slots)) {
        ret = 0;
        bsd_errno = Errno::SUCCESS;
        for (const MMsgSlot& slot : slots) {
            std::vector<u8> data;
            for (const auto& [offset, len] : slot.iovs) {
                data.insert(data.end(), buffer.begin() + offset, buffer.begin() + offset + len);
            }
            const auto [sent, err] =
                slot.name_len >= 8
                    ? SendToImpl(fd, flags, data,
                                 std::span<const u8>{buffer.data() + slot.name_offset,
                                                     slot.name_len})
                    : SendImpl(fd, flags, data);
            if (sent < 0) {
                if (ret == 0) {
                    ret = -1;
                    bsd_errno = err;
                }
                break;
            }
            const u32 msg_len = static_cast<u32>(sent);
            std::memcpy(buffer.data() + slot.flags_offset + sizeof(u32), &msg_len, sizeof(u32));
            ret++;
            if (static_cast<size_t>(sent) < data.size()) {
                break;
            }
        }
        ctx.WriteBufferB(buffer.data(), buffer.size());
    }
    LOG_DEBUG(Service, "called. fd={} vlen={} flags={:#x} ret={} errno={}", fd, vlen, flags, ret,
              bsd_errno);

    IPC::ResponseBuilder rb{ctx, 4};
    rb.Push(ResultSuccess);
    rb.Push<s32>(ret);
    rb.PushEnum(bsd_errno);
}

void BSD_USA::RecvMMsg(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const s32 fd = rp.Pop<s32>();
    const u32 vlen = rp.Pop<u32>();
    const u32 flags = rp.Pop<u32>();
    // A 16-byte timeout follows; the socket's blocking mode decides instead.

    std::vector<u8> buffer = ReadBufferB(ctx);
    std::vector<MMsgSlot> slots;
    s32 ret = -1;
    Errno bsd_errno = Errno::INVAL;
    if (ParseMMsgBuffer(buffer, vlen, slots)) {
        ret = 0;
        bsd_errno = Errno::SUCCESS;
        for (const MMsgSlot& slot : slots) {
            size_t capacity = 0;
            for (const auto& [offset, len] : slot.iovs) {
                capacity += len;
            }
            std::vector<u8> data(capacity);
            std::vector<u8> addr(slot.name_len >= 16 ? 16 : 0);
            const auto [received, err] = addr.empty() ? RecvImpl(fd, flags, data)
                                                      : RecvFromImpl(fd, flags, data, addr);
            if (received < 0) {
                if (ret == 0) {
                    ret = -1;
                    bsd_errno = err;
                }
                break;
            }
            size_t copied = 0;
            for (const auto& [offset, len] : slot.iovs) {
                const size_t n = std::min<size_t>(len, static_cast<size_t>(received) - copied);
                std::memcpy(buffer.data() + offset, data.data() + copied, n);
                copied += n;
            }
            if (!addr.empty()) {
                std::memcpy(buffer.data() + slot.name_offset, addr.data(), addr.size());
            }
            std::memset(buffer.data() + slot.control_offset, 0, slot.control_len);
            const u32 msg_flags = 0;
            const u32 msg_len = static_cast<u32>(received);
            std::memcpy(buffer.data() + slot.flags_offset, &msg_flags, sizeof(u32));
            std::memcpy(buffer.data() + slot.flags_offset + sizeof(u32), &msg_len, sizeof(u32));
            ret++;
            break; // one datagram or stream read per call is enough for the SDK's recvmsg
        }
        ctx.WriteBufferB(buffer.data(), buffer.size());
    }
    LOG_DEBUG(Service, "called. fd={} vlen={} flags={:#x} ret={} errno={}", fd, vlen, flags, ret,
              bsd_errno);

    IPC::ResponseBuilder rb{ctx, 4};
    rb.Push(ResultSuccess);
    rb.Push<s32>(ret);
    rb.PushEnum(bsd_errno);
}

void BSD_USA::StartMonitoring(HLERequestContext& ctx) {
    LOG_WARNING(Service, "(STUBBED) called");

    IPC::ResponseBuilder rb{ctx, 2};

    rb.Push(ResultSuccess);
}

void BSD_USA::Socket(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const u32 domain = rp.Pop<u32>();
    const u32 type = rp.Pop<u32>();
    const u32 protocol = rp.Pop<u32>();

    LOG_DEBUG(Service, "called. domain={} type={} protocol={}", domain, type, protocol);

    const auto [fd, bsd_errno] = SocketImpl(Domain(domain), Type(type), Protocol(protocol));

    IPC::ResponseBuilder rb{ctx, 4};
    rb.Push(ResultSuccess);
    rb.Push<s32>(fd);
    rb.PushEnum(bsd_errno);
}

void BSD_USA::SocketExempt(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const u32 domain = rp.Pop<u32>();
    const u32 type = rp.Pop<u32>();
    const u32 protocol = rp.Pop<u32>();

    LOG_DEBUG(Service, "called. domain={} type={} protocol={}", domain, type, protocol);

    auto [fd, bsd_errno] = SocketImpl(Domain(domain), Type(type), Protocol(protocol));
    if (bsd_errno == Errno::SUCCESS) {
        bsd_errno = ShutdownImpl(fd, 0);
    }

    IPC::ResponseBuilder rb{ctx, 4};
    rb.Push(ResultSuccess);
    rb.Push<s32>(fd);
    rb.PushEnum(bsd_errno);
}

void BSD_USA::Select(HLERequestContext& ctx) {
    LOG_DEBUG(Service, "(STUBBED) called");

    IPC::ResponseBuilder rb{ctx, 4};

    rb.Push(ResultSuccess);
    rb.Push<u32>(0); // ret
    rb.Push<u32>(0); // bsd errno
}

void BSD_USA::Poll(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const s32 nfds = rp.Pop<s32>();
    const s32 timeout = rp.Pop<s32>();

    LOG_DEBUG(Service, "called. nfds={} timeout={}", nfds, timeout);

    ExecuteWork(ctx, PollWork{
                         .nfds = nfds,
                         .timeout = timeout,
                         .read_buffer = ctx.ReadBuffer(),
                         .write_buffer = std::vector<u8>(ctx.GetWriteBufferSize()),
                     });
}

void BSD_USA::Accept(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const s32 fd = rp.Pop<s32>();

    LOG_DEBUG(Service, "called. fd={}", fd);

    ExecuteWork(ctx, AcceptWork{
                         .fd = fd,
                         .write_buffer = std::vector<u8>(ctx.GetWriteBufferSize()),
                     });
}

void BSD_USA::Bind(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const s32 fd = rp.Pop<s32>();

    LOG_DEBUG(Service, "called. fd={} addrlen={}", fd, ctx.GetReadBufferSize());
    BuildErrnoResponse(ctx, BindImpl(fd, ctx.ReadBuffer()));
}

void BSD_USA::Connect(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const s32 fd = rp.Pop<s32>();

    LOG_DEBUG(Service, "called. fd={} addrlen={}", fd, ctx.GetReadBufferSize());

    ExecuteWork(ctx, ConnectWork{
                         .fd = fd,
                         .addr = ctx.ReadBuffer(),
                     });
}

void BSD_USA::GetPeerName(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const s32 fd = rp.Pop<s32>();

    LOG_DEBUG(Service, "called. fd={}", fd);

    std::vector<u8> write_buffer(ctx.GetWriteBufferSize());
    const Errno bsd_errno = GetPeerNameImpl(fd, write_buffer);

    ctx.WriteBuffer(write_buffer);

    IPC::ResponseBuilder rb{ctx, 5};
    rb.Push(ResultSuccess);
    rb.Push<s32>(bsd_errno != Errno::SUCCESS ? -1 : 0);
    rb.PushEnum(bsd_errno);
    rb.Push<u32>(static_cast<u32>(write_buffer.size()));
}

void BSD_USA::GetSockName(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const s32 fd = rp.Pop<s32>();

    LOG_DEBUG(Service, "called. fd={}", fd);

    std::vector<u8> write_buffer(ctx.GetWriteBufferSize());
    const Errno bsd_errno = GetSockNameImpl(fd, write_buffer);

    ctx.WriteBuffer(write_buffer);

    IPC::ResponseBuilder rb{ctx, 5};
    rb.Push(ResultSuccess);
    rb.Push<s32>(bsd_errno != Errno::SUCCESS ? -1 : 0);
    rb.PushEnum(bsd_errno);
    rb.Push<u32>(static_cast<u32>(write_buffer.size()));
}

void BSD_USA::GetSockOpt(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const s32 fd = rp.Pop<s32>();
    const u32 level = rp.Pop<u32>();
    const auto optname = static_cast<OptName>(rp.Pop<u32>());

    std::vector<u8> optval(ctx.GetWriteBufferSize());

    LOG_DEBUG(Service, "called. fd={} level={} optname={:#x} len={:#x}", fd, level, optname,
              optval.size());

    const Errno err = GetSockOptImpl(fd, level, optname, optval);

    ctx.WriteBuffer(optval);

    IPC::ResponseBuilder rb{ctx, 5};
    rb.Push(ResultSuccess);
    rb.Push<s32>(err == Errno::SUCCESS ? 0 : -1);
    rb.PushEnum(err);
    rb.Push<u32>(static_cast<u32>(optval.size()));
}

void BSD_USA::Listen(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const s32 fd = rp.Pop<s32>();
    const s32 backlog = rp.Pop<s32>();

    LOG_DEBUG(Service, "called. fd={} backlog={}", fd, backlog);

    BuildErrnoResponse(ctx, ListenImpl(fd, backlog));
}

void BSD_USA::Fcntl(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const s32 fd = rp.Pop<s32>();
    const s32 cmd = rp.Pop<s32>();
    const s32 arg = rp.Pop<s32>();

    LOG_DEBUG(Service, "called. fd={} cmd={} arg={}", fd, cmd, arg);

    const auto [ret, bsd_errno] = FcntlImpl(fd, static_cast<FcntlCmd>(cmd), arg);

    IPC::ResponseBuilder rb{ctx, 4};
    rb.Push(ResultSuccess);
    rb.Push<s32>(ret);
    rb.PushEnum(bsd_errno);
}

void BSD_USA::SetSockOpt(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};

    const s32 fd = rp.Pop<s32>();
    const u32 level = rp.Pop<u32>();
    const OptName optname = static_cast<OptName>(rp.Pop<u32>());
    const auto optval = ctx.ReadBuffer();

    LOG_DEBUG(Service, "called. fd={} level={} optname={:#x} optlen={}", fd, level,
              static_cast<u32>(optname), optval.size());

    BuildErrnoResponse(ctx, SetSockOptImpl(fd, level, optname, optval));
}

void BSD_USA::Shutdown(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};

    const s32 fd = rp.Pop<s32>();
    const s32 how = rp.Pop<s32>();

    LOG_DEBUG(Service, "called. fd={} how={}", fd, how);

    BuildErrnoResponse(ctx, ShutdownImpl(fd, how));
}

void BSD_USA::Recv(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};

    const s32 fd = rp.Pop<s32>();
    const u32 flags = rp.Pop<u32>();

    LOG_DEBUG(Service, "called. fd={} flags={:#x} len={}", fd, flags, ctx.GetWriteBufferSize());

    ExecuteWork(ctx, RecvWork{
                         .fd = fd,
                         .flags = flags,
                         .message = std::vector<u8>(ctx.GetWriteBufferSize()),
                     });
}

void BSD_USA::RecvFrom(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};

    const s32 fd = rp.Pop<s32>();
    const u32 flags = rp.Pop<u32>();

    LOG_DEBUG(Service, "called. fd={} flags={:#x} len={} addrlen={}", fd, flags,
              ctx.GetWriteBufferSize(0), ctx.GetWriteBufferSize(1));

    ExecuteWork(ctx, RecvFromWork{
                         .fd = fd,
                         .flags = flags,
                         .message = std::vector<u8>(ctx.GetWriteBufferSize(0)),
                         .addr = std::vector<u8>(ctx.GetWriteBufferSize(1)),
                     });
}

void BSD_USA::Send(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};

    const s32 fd = rp.Pop<s32>();
    const u32 flags = rp.Pop<u32>();

    LOG_DEBUG(Service, "called. fd={} flags={:#x} len={}", fd, flags, ctx.GetReadBufferSize());

    ExecuteWork(ctx, SendWork{
                         .fd = fd,
                         .flags = flags,
                         .message = ctx.ReadBuffer(),
                     });
}

void BSD_USA::SendTo(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const s32 fd = rp.Pop<s32>();
    const u32 flags = rp.Pop<u32>();

    LOG_DEBUG(Service, "called. fd={} flags=0x{} len={} addrlen={}", fd, flags,
              ctx.GetReadBufferSize(0), ctx.GetReadBufferSize(1));

    ExecuteWork(ctx, SendToWork{
                         .fd = fd,
                         .flags = flags,
                         .message = ctx.ReadBuffer(0),
                         .addr = ctx.ReadBuffer(1),
                     });
}

void BSD_USA::Write(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const s32 fd = rp.Pop<s32>();

    LOG_DEBUG(Service, "called. fd={} len={}", fd, ctx.GetReadBufferSize());

    ExecuteWork(ctx, SendWork{
                         .fd = fd,
                         .flags = 0,
                         .message = ctx.ReadBuffer(),
                     });
}

void BSD_USA::Read(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const s32 fd = rp.Pop<s32>();

    LOG_DEBUG(Service, "called. fd={} len={}", fd, ctx.GetWriteBufferSize());

    ExecuteWork(ctx, RecvWork{
                         .fd = fd,
                         .flags = 0,
                         .message = std::vector<u8>(ctx.GetWriteBufferSize()),
                     });
}

void BSD_USA::Close(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const s32 fd = rp.Pop<s32>();

    LOG_DEBUG(Service, "called. fd={}", fd);

    BuildErrnoResponse(ctx, CloseImpl(fd));
}

/// @brief Only bsd:s is able to dup()
void BSD_USA::DuplicateSocket(HLERequestContext& ctx) {
    struct InputParameters {
        s32 fd;
        u64 reserved;
    };
    static_assert(sizeof(InputParameters) == 0x10);

    struct OutputParameters {
        s32 ret;
        Errno bsd_errno;
    };
    static_assert(sizeof(OutputParameters) == 0x8);

    IPC::RequestParser rp{ctx};
    auto input = rp.PopRaw<InputParameters>();

    IPC::ResponseBuilder rb{ctx, 4};
    rb.Push(ResultSuccess);
    if (is_user) {
        rb.PushRaw(OutputParameters{
            .ret = 0,
            .bsd_errno = Errno::INVAL,
        });
        return;
    }

    auto const res_v = DuplicateSocketImpl(input.fd);
    if (auto* res = std::get_if<s32>(&res_v)) {
        rb.PushRaw(OutputParameters{
            .ret = *res,
            .bsd_errno = Errno::SUCCESS,
        });
    } else {
        auto* err = std::get_if<Errno>(&res_v);
        rb.PushRaw(OutputParameters{
            .ret = 0,
            .bsd_errno = *err,
        });
    }
}

void BSD_USA::EventFd(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const u64 initval = rp.Pop<u64>();
    const u32 flags = rp.Pop<u32>();

#if defined(__linux__) || defined(_WIN32)
    const s32 fd = FindFreeFileDescriptorHandle();
    auto socket = std::make_shared<EventFdSocket>(initval, (flags & 1) != 0);
    if (fd < 0 || !socket->IsOpened()) {
        LOG_ERROR(Service, "EventFd failed: initval={} flags={:#x}", initval, flags);
        IPC::ResponseBuilder rb{ctx, 4};
        rb.Push(ResultSuccess);
        rb.Push<s32>(-1);
        rb.PushEnum(Errno::MFILE);
        return;
    }
    file_descriptors[fd] = FileDescriptor{
        .socket = std::move(socket),
        .flags = 0,
        .is_connection_based = false,
    };
    LOG_INFO(Service, "EventFd fd={} initval={} flags={:#x}", fd, initval, flags);

    IPC::ResponseBuilder rb{ctx, 4};
    rb.Push(ResultSuccess);
    rb.Push<s32>(fd);
    rb.PushEnum(Errno::SUCCESS);
#else
    LOG_WARNING(Service, "(STUBBED) called. initval={}, flags={}", initval, flags);

    BuildErrnoResponse(ctx, Errno::SUCCESS);
#endif
}

template <typename Work>
void BSD_USA::ExecuteWork(HLERequestContext& ctx, Work work) {
    work.Execute(this);
    work.Response(ctx);
}

std::pair<s32, Errno> BSD_USA::SocketImpl(Domain domain, Type type, Protocol protocol) {
    // user bsd:u has restrictions on SOCK_SEQPACKET and SOCK_RAW
    if (is_user && (type == Type::SEQPACKET || type == Type::RAW)) {
        if (type == Type::RAW && domain == Domain::INET && protocol == Protocol::ICMP) {
            // fine, can use on bsd:s and bsd:u
        } else {
            return {-1, Errno::INVAL};
        }
    }

    [[maybe_unused]] const bool unk_flag = (static_cast<u32>(type) & 0x20000000) != 0;
    UNIMPLEMENTED_IF_MSG(unk_flag, "Unknown flag in type");
    type = static_cast<Type>(static_cast<u32>(type) & ~0x20000000);

    const s32 fd = FindFreeFileDescriptorHandle();
    if (fd < 0) {
        LOG_ERROR(Service, "No more file descriptors available");
        return {-1, Errno::MFILE};
    }

    file_descriptors[fd] = FileDescriptor{};
    FileDescriptor& descriptor = *file_descriptors[fd];
    // ENONMEM might be thrown here

    LOG_INFO(Service, "New socket fd={}", fd);

#ifdef __linux__
    if (domain == Domain::INET && type == Type::RAW && protocol == Protocol::ICMP) {
        descriptor.socket = std::make_shared<IcmpPingSocket>();
        LOG_INFO(Service, "fd={} is a raw ICMP socket, backed by a host ping socket ({})", fd,
                 descriptor.socket->IsOpened() ? "open" : "unavailable");
        return {fd, Errno::SUCCESS};
    }
#endif

    auto room_member = Network::GetRoomMember().lock();
    if (room_member && room_member->IsConnected()) {
        descriptor.socket = std::make_shared<Network::ProxySocket>();
    } else {
        descriptor.socket = std::make_shared<Network::Socket>();
    }

    descriptor.socket->Initialize(Translate(domain), Translate(type), Translate(protocol));
    descriptor.is_connection_based = IsConnectionBased(type);

    if (Settings::values.airplane_mode.GetValue() && descriptor.is_connection_based) {
        LOG_ERROR(Service, "Airplane mode is enabled, cannot create socket");
        return {-1, Errno::NOTCONN};
    }

    return {fd, Errno::SUCCESS};
}

std::pair<s32, Errno> BSD_USA::PollImpl(std::vector<u8>& write_buffer, std::span<const u8> read_buffer,
                                    s32 nfds, s32 timeout) {
    if (nfds <= 0) {
        // When no entries are provided, -1 is returned with errno zero
        return {-1, Errno::SUCCESS};
    }
    if (read_buffer.size() < nfds * sizeof(PollFD)) {
        return {-1, Errno::INVAL};
    }
    if (write_buffer.size() < nfds * sizeof(PollFD)) {
        return {-1, Errno::INVAL};
    }

    std::vector<PollFD> fds(nfds);
    std::memcpy(fds.data(), read_buffer.data(), nfds * sizeof(PollFD));

    if (timeout >= 0) {
        const s64 seconds = timeout / 1000;
        const u64 nanoseconds = 1'000'000 * (static_cast<u64>(timeout) % 1000);

        if (seconds < 0) {
            return {-1, Errno::INVAL};
        }
        if (nanoseconds > 999'999'999) {
            return {-1, Errno::INVAL};
        }
    } else if (timeout != -1) {
        return {-1, Errno::INVAL};
    }

    for (PollFD& pollfd : fds) {
        ASSERT(False(pollfd.revents));

        if (pollfd.fd > static_cast<s32>(MAX_FD) || pollfd.fd < 0) {
            LOG_ERROR(Service, "File descriptor handle={} is invalid", pollfd.fd);
            pollfd.revents = PollEvents{};
            return {0, Errno::SUCCESS};
        }

        const std::optional<FileDescriptor>& descriptor = file_descriptors[pollfd.fd];
        if (!descriptor) {
            LOG_TRACE(Service, "File descriptor handle={} is not allocated", pollfd.fd);
            pollfd.revents = PollEvents::Nval;
            return {0, Errno::SUCCESS};
        }
    }

    std::vector<Network::PollFD> host_pollfds(fds.size());
    std::transform(fds.begin(), fds.end(), host_pollfds.begin(), [](PollFD pollfd) {
        Network::PollFD result;
        result.socket = file_descriptors[pollfd.fd]->socket.get();
        result.events = Translate(pollfd.events);
        result.revents = Network::PollEvents{};
        return result;
    });

    // bsd requests run one at a time on the service thread, so a guest Poll that waits forever
    // blocks every other bsd call, including the eventfd Write that would wake it. gRPC polls
    // that way (Raiders' NPLN client deadlocked before connecting). In private-server mode, wait
    // at most a short slice and report a timeout; the guest polls again, and calls queued from
    // other threads run in between.
    s32 host_timeout = timeout;
    if (OnlineRedirect::Get().enabled) {
        constexpr s32 PollSliceMs = 10;
        if (timeout < 0 || timeout > PollSliceMs) {
            host_timeout = PollSliceMs;
        }
    }
    const auto result = Network::Poll(host_pollfds, host_timeout);

    const size_t num = host_pollfds.size();
    for (size_t i = 0; i < num; ++i) {
        fds[i].revents = Translate(host_pollfds[i].revents);
    }
    std::memcpy(write_buffer.data(), fds.data(), nfds * sizeof(PollFD));

    return Translate(result);
}

std::pair<s32, Errno> BSD_USA::AcceptImpl(s32 fd, std::vector<u8>& write_buffer) {
    if (!IsFileDescriptorValid(fd)) {
        return {-1, Errno::BADF};
    }

    const s32 new_fd = FindFreeFileDescriptorHandle();
    if (new_fd < 0) {
        LOG_ERROR(Service, "No more file descriptors available");
        return {-1, Errno::MFILE};
    }

    FileDescriptor& descriptor = *file_descriptors[fd];
    auto [result, bsd_errno] = descriptor.socket->Accept();
    if (bsd_errno != Network::Errno::SUCCESS) {
        return {-1, Translate(bsd_errno)};
    }

    file_descriptors[new_fd] = FileDescriptor{};
    FileDescriptor& new_descriptor = *file_descriptors[new_fd];
    new_descriptor.socket = std::move(result.socket);
    new_descriptor.is_connection_based = descriptor.is_connection_based;

    const SockAddrIn guest_addr_in = Translate(result.sockaddr_in);
    PutValue(write_buffer, guest_addr_in);

    return {new_fd, Errno::SUCCESS};
}

// Debug aid for the private-server work (Pia's peer-to-peer traffic): with EDEN_NX2_UDP_TRACE set,
// log every bind and the first few datagrams per socket and peer address.
static bool UdpTraceEnabled() {
    static const bool enabled = std::getenv("EDEN_NX2_UDP_TRACE") != nullptr;
    return enabled;
}

// EDEN_NX2_UDP_TRACE=all logs every datagram with its first bytes, not just the first four per peer.
static bool UdpTraceAll() {
    static const bool all = [] {
        const char* value = std::getenv("EDEN_NX2_UDP_TRACE");
        return value != nullptr && std::string_view{value} == "all";
    }();
    return all;
}

static void UdpTrace(const char* what, s32 fd, const Network::SockAddrIn& addr, s64 size,
                     std::span<const u8> data = {}) {
    static std::mutex mutex;
    static std::map<std::tuple<std::string, s32, std::array<u8, 4>, u16>, int> counts;
    std::scoped_lock lock{mutex};
    int& count = counts[{what, fd, addr.ip, addr.portno}];
    if (count++ < 4 || UdpTraceAll()) {
        LOG_INFO(Service, "UDP trace: {} fd={} {}:{} size={} {}", what, fd,
                 Network::IPv4AddressToString(addr.ip), addr.portno, size,
                 Common::HexToString(data.first(std::min<size_t>(data.size(), 32)), false));
    }
}

Errno BSD_USA::BindImpl(s32 fd, std::span<const u8> addr) {
    if (!IsFileDescriptorValid(fd)) {
        return Errno::BADF;
    }
    ASSERT(addr.size() >= 16);
    if (!file_descriptors[fd]->socket) {
        LOG_WARNING(Service, "Uninitialized socket");
        return Errno::BADF;
    }

    auto addr_in = GetValue<SockAddrIn>(addr);

    const Errno result = Translate(file_descriptors[fd]->socket->Bind(Translate(addr_in)));
    if (UdpTraceEnabled()) {
        UdpTrace(result == Errno::SUCCESS ? "bind" : "bind-failed", fd, Translate(addr_in), 0);
    }
    return result;
}

Errno BSD_USA::ConnectImpl(s32 fd, std::span<const u8> addr) {
    if (!IsFileDescriptorValid(fd)) {
        return Errno::BADF;
    }

    ASSERT(addr.size() >= 16);
    if (!file_descriptors[fd]->socket) {
        LOG_WARNING(Service, "Uninitialized socket");
        return Errno::BADF;
    }

    Network::SockAddrIn addr_in = Translate(GetValue<SockAddrIn>(addr));
    if (const auto& online = OnlineRedirect::Get(); online.enabled) {
        const bool allowed = file_descriptors[fd]->is_connection_based
                                 ? OnlineRedirect::IsAllowedDestination(addr_in.ip)
                                 : OnlineRedirect::IsAllowedPeerDestination(addr_in.ip);
        if (!allowed) {
            LOG_WARNING(Service, "Online redirect: refusing connection to {}:{}",
                        Network::IPv4AddressToString(addr_in.ip), addr_in.portno);
            return Errno::CONNREFUSED;
        }
        if (addr_in.ip == online.ip && addr_in.portno == 443) {
            addr_in.portno = online.port;
        }
    }

    const Errno result = Translate(file_descriptors[fd]->socket->Connect(addr_in));

    if (result == Errno::ISCONN) {
        LOG_DEBUG(Service, "returned ISCONN - socket already connected");
        return Errno::SUCCESS;
    }

    return result;
}

Errno BSD_USA::GetPeerNameImpl(s32 fd, std::vector<u8>& write_buffer) {
    if (!IsFileDescriptorValid(fd)) {
        return Errno::BADF;
    }

    if (!file_descriptors[fd]->socket) {
        LOG_WARNING(Service, "Uninitialized socket");
        return Errno::BADF;
    }

    const auto [addr_in, bsd_errno] = file_descriptors[fd]->socket->GetPeerName();
    if (bsd_errno != Network::Errno::SUCCESS) {
        return Translate(bsd_errno);
    }
    const SockAddrIn guest_addrin = Translate(addr_in);

    ASSERT(write_buffer.size() >= sizeof(guest_addrin));
    write_buffer.resize(sizeof(guest_addrin));
    PutValue(write_buffer, guest_addrin);
    return Translate(bsd_errno);
}

Errno BSD_USA::GetSockNameImpl(s32 fd, std::vector<u8>& write_buffer) {
    if (!IsFileDescriptorValid(fd)) {
        return Errno::BADF;
    }

    if (!file_descriptors[fd]->socket) {
        LOG_WARNING(Service, "Uninitialized socket");
        return Errno::BADF;
    }

    const auto [addr_in, bsd_errno] = file_descriptors[fd]->socket->GetSockName();
    if (bsd_errno != Network::Errno::SUCCESS) {
        return Translate(bsd_errno);
    }
    const SockAddrIn guest_addrin = Translate(addr_in);

    ASSERT(write_buffer.size() >= sizeof(guest_addrin));
    write_buffer.resize(sizeof(guest_addrin));
    PutValue(write_buffer, guest_addrin);
    return Translate(bsd_errno);
}

Errno BSD_USA::ListenImpl(s32 fd, s32 backlog) {
    if (!IsFileDescriptorValid(fd)) {
        return Errno::BADF;
    }
    if (!file_descriptors[fd]->socket) {
        LOG_WARNING(Service, "Uninitialized socket");
        return Errno::BADF;
    }
    return Translate(file_descriptors[fd]->socket->Listen(backlog));
}

std::pair<s32, Errno> BSD_USA::FcntlImpl(s32 fd, FcntlCmd cmd, s32 arg) {
    if (!IsFileDescriptorValid(fd)) {
        return {-1, Errno::BADF};
    }
    if (!file_descriptors[fd]->socket) {
        LOG_WARNING(Service, "Uninitialized socket");
        return {-1, Errno::BADF};
    }

    FileDescriptor& descriptor = *file_descriptors[fd];

    switch (cmd) {
    case FcntlCmd::GETFL:
        ASSERT(arg == 0);
        return {descriptor.flags, Errno::SUCCESS};
    case FcntlCmd::SETFL: {
        const bool enable = (arg & Network::FLAG_O_NONBLOCK) != 0;
        const Errno bsd_errno = Translate(descriptor.socket->SetNonBlock(enable));
        if (bsd_errno != Errno::SUCCESS) {
            return {-1, bsd_errno};
        }
        descriptor.flags = arg;
        return {0, Errno::SUCCESS};
    }
    default:
        UNIMPLEMENTED_MSG("Unimplemented cmd={}", cmd);
        return {-1, Errno::SUCCESS};
    }
}

Errno BSD_USA::GetSockOptImpl(s32 fd, u32 level, OptName optname, std::vector<u8>& optval) {
    if (!IsFileDescriptorValid(fd)) {
        return Errno::BADF;
    }
    if (!file_descriptors[fd]->socket) {
        LOG_WARNING(Service, "Uninitialized socket");
        return Errno::BADF;
    }

    // Options read back after a set (gRPC checks TCP_NODELAY and SO_REUSEADDR that way and drops
    // the socket when the value differs) return what was set; Eden used to return zeros.
    if (optname != OptName::ERROR_) {
        const auto& stored = file_descriptors[fd]->options;
        if (const auto it = stored.find(OptionKey(level, optname)); it != stored.end()) {
            optval.assign(it->second.begin(), it->second.end());
            return Errno::SUCCESS;
        }
    }

    if (level != static_cast<u32>(SocketLevel::SOCKET)) {
        UNIMPLEMENTED_MSG("Unknown getsockopt level");
        return Errno::SUCCESS;
    }

    Network::SocketBase* const socket = file_descriptors[fd]->socket.get();

    switch (optname) {
    case OptName::ERROR_: {
        auto [pending_err, getsockopt_err] = socket->GetPendingError();
        if (getsockopt_err == Network::Errno::SUCCESS) {
            Errno translated_pending_err = Translate(pending_err);
            ASSERT_OR_EXECUTE_MSG(
                optval.size() == sizeof(Errno), { return Errno::INVAL; },
                "Incorrect getsockopt option size");
            optval.resize(sizeof(Errno));
            PutValue(optval, translated_pending_err);
        }
        return Translate(getsockopt_err);
    }
    default:
        UNIMPLEMENTED_MSG("Unimplemented optname={}", optname);
        return Errno::SUCCESS;
    }
}

Errno BSD_USA::SetSockOptImpl(s32 fd, u32 level, OptName optname, std::span<const u8> optval) {
    if (!IsFileDescriptorValid(fd)) {
        return Errno::BADF;
    }
    if (!file_descriptors[fd]->socket) {
        LOG_WARNING(Service, "Uninitialized socket");
        return Errno::BADF;
    }

    file_descriptors[fd]->options[OptionKey(level, optname)].assign(optval.begin(), optval.end());

    if (level == TcpLevel && static_cast<u32>(optname) == TcpNoDelay && optval.size() >= sizeof(u32)) {
        const bool enable = GetValue<u32>(optval) != 0;
#ifdef _WIN32
        const SOCKET host_fd = file_descriptors[fd]->socket->GetFD();
        const BOOL host_value = enable ? TRUE : FALSE;
        if (host_fd != INVALID_SOCKET) {
            ::setsockopt(host_fd, IPPROTO_TCP, TCP_NODELAY,
                         reinterpret_cast<const char*>(&host_value), sizeof(host_value));
        }
#else
        const int host_fd = file_descriptors[fd]->socket->GetFD();
        const int host_value = enable ? 1 : 0;
        if (host_fd >= 0) {
            ::setsockopt(host_fd, IPPROTO_TCP, TCP_NODELAY, &host_value, sizeof(host_value));
        }
#endif
        return Errno::SUCCESS;
    }

    if (level != static_cast<u32>(SocketLevel::SOCKET)) {
        LOG_WARNING(Service, "(STUBBED) setsockopt with level={}, optname={}", level, optname);
        return Errno::SUCCESS;
    }

    Network::SocketBase* const socket = file_descriptors[fd]->socket.get();

    // Switch 2 sockets set options Eden doesn't know (0x10000, and 0x80000001 with 8 bytes, right
    // after socket()); accept them instead of asserting on their size.
    switch (optname) {
    case OptName::LINGER:
    case OptName::REUSEADDR:
    case OptName::KEEPALIVE:
    case OptName::BROADCAST:
    case OptName::SNDBUF:
    case OptName::RCVBUF:
    case OptName::SNDTIMEO:
    case OptName::RCVTIMEO:
    case OptName::NOSIGPIPE:
        break;
    default:
        LOG_WARNING(Service, "(STUBBED) setsockopt optname={:#x} size={}",
                    static_cast<u32>(optname), optval.size());
        return Errno::SUCCESS;
    }

    if (optname == OptName::LINGER) {
        ASSERT(optval.size() == sizeof(Linger));
        auto linger = GetValue<Linger>(optval);
        ASSERT(linger.onoff == 0 || linger.onoff == 1);

        return Translate(socket->SetLinger(linger.onoff != 0, linger.linger));
    }

    ASSERT(optval.size() == sizeof(u32));
    auto value = GetValue<u32>(optval);

    switch (optname) {
    case OptName::REUSEADDR:
        ASSERT(value == 0 || value == 1);
        return Translate(socket->SetReuseAddr(value != 0));
    case OptName::KEEPALIVE:
        ASSERT(value == 0 || value == 1);
        return Translate(socket->SetKeepAlive(value != 0));
    case OptName::BROADCAST:
        ASSERT(value == 0 || value == 1);
        return Translate(socket->SetBroadcast(value != 0));
    case OptName::SNDBUF:
        return Translate(socket->SetSndBuf(value));
    case OptName::RCVBUF:
        return Translate(socket->SetRcvBuf(value));
    case OptName::SNDTIMEO:
        return Translate(socket->SetSndTimeo(value));
    case OptName::RCVTIMEO:
        return Translate(socket->SetRcvTimeo(value));
    case OptName::NOSIGPIPE:
        LOG_WARNING(Service, "(STUBBED) setting NOSIGPIPE to {}", value);
        return Errno::SUCCESS;
    default:
        UNIMPLEMENTED_MSG("Unimplemented optname={}", optname);
        return Errno::SUCCESS;
    }
}

Errno BSD_USA::ShutdownImpl(s32 fd, s32 how) {
    if (!IsFileDescriptorValid(fd)) {
        return Errno::BADF;
    }
    if (!file_descriptors[fd]->socket) {
        LOG_WARNING(Service, "Uninitialized socket");
        return Errno::BADF;
    }
    const Network::ShutdownHow host_how = Translate(static_cast<ShutdownHow>(how));
    return Translate(file_descriptors[fd]->socket->Shutdown(host_how));
}

std::pair<s32, Errno> BSD_USA::RecvImpl(s32 fd, u32 flags, std::vector<u8>& message) {
    if (!IsFileDescriptorValid(fd)) {
        return {-1, Errno::BADF};
    }

    FileDescriptor& descriptor = *file_descriptors[fd];

    // Apply flags
    using Network::FLAG_MSG_DONTWAIT;
    using Network::FLAG_O_NONBLOCK;
    if ((flags & FLAG_MSG_DONTWAIT) != 0) {
        flags &= ~FLAG_MSG_DONTWAIT;
        if ((descriptor.flags & FLAG_O_NONBLOCK) == 0) {
            descriptor.socket->SetNonBlock(true);
        }
    }

    const auto [ret, bsd_errno] = Translate(descriptor.socket->Recv(flags, message));

    // Restore original state
    if ((descriptor.flags & FLAG_O_NONBLOCK) == 0) {
        descriptor.socket->SetNonBlock(false);
    }

    return {ret, bsd_errno};
}

std::pair<s32, Errno> BSD_USA::RecvFromImpl(s32 fd, u32 flags, std::vector<u8>& message,
                                        std::vector<u8>& addr) {
    if (!IsFileDescriptorValid(fd)) {
        return {-1, Errno::BADF};
    }

    FileDescriptor& descriptor = *file_descriptors[fd];

    Network::SockAddrIn addr_in{};
    Network::SockAddrIn* p_addr_in = nullptr;
    if (descriptor.is_connection_based) {
        // Connection based file descriptors (e.g. TCP) zero addr
        addr.clear();
    } else {
        p_addr_in = &addr_in;
    }

    // Apply flags
    using Network::FLAG_MSG_DONTWAIT;
    using Network::FLAG_O_NONBLOCK;
    if ((flags & FLAG_MSG_DONTWAIT) != 0) {
        flags &= ~FLAG_MSG_DONTWAIT;
        if ((descriptor.flags & FLAG_O_NONBLOCK) == 0) {
            descriptor.socket->SetNonBlock(true);
        }
    }

    const auto [ret, bsd_errno] = Translate(descriptor.socket->RecvFrom(flags, message, p_addr_in));

    // Restore original state
    if ((descriptor.flags & FLAG_O_NONBLOCK) == 0) {
        descriptor.socket->SetNonBlock(false);
    }

    if (p_addr_in) {
        if (ret < 0) {
            addr.clear();
        } else {
            ASSERT(addr.size() >= 16);
            const SockAddrIn result = Translate(addr_in);
            PutValue(addr, result);
            if (UdpTraceEnabled()) {
                UdpTrace("recvfrom", fd, addr_in, ret,
                         std::span<const u8>{message}.first(static_cast<size_t>(ret)));
            }
        }
    }

    return {ret, bsd_errno};
}

std::pair<s32, Errno> BSD_USA::SendImpl(s32 fd, u32 flags, std::span<const u8> message) {
    if (!IsFileDescriptorValid(fd)) {
        return {-1, Errno::BADF};
    }
    if (!file_descriptors[fd]->socket) {
        LOG_WARNING(Service, "Uninitialized socket");
        return {-1, Errno::BADF};
    }
    return Translate(file_descriptors[fd]->socket->Send(message, flags));
}

std::pair<s32, Errno> BSD_USA::SendToImpl(s32 fd, u32 flags, std::span<const u8> message,
                                      std::span<const u8> addr) {
    if (!IsFileDescriptorValid(fd)) {
        return {-1, Errno::BADF};
    }
    if (!file_descriptors[fd]->socket) {
        LOG_WARNING(Service, "Uninitialized socket");
        return {-1, Errno::BADF};
    }

    Network::SockAddrIn addr_in;
    Network::SockAddrIn* p_addr_in = nullptr;
    if (!addr.empty()) {
        ASSERT(addr.size() >= 16);
        auto guest_addr_in = GetValue<SockAddrIn>(addr);
        addr_in = Translate(guest_addr_in);
        p_addr_in = &addr_in;
        if (OnlineRedirect::Get().enabled && !OnlineRedirect::IsAllowedPeerDestination(addr_in.ip)) {
            LOG_WARNING(Service, "Online redirect: dropping datagram to {}:{}",
                        Network::IPv4AddressToString(addr_in.ip), addr_in.portno);
            return {-1, Errno::CONNREFUSED};
        }
    }

    const auto result = Translate(file_descriptors[fd]->socket->SendTo(flags, message, p_addr_in));
    if (UdpTraceEnabled() && p_addr_in) {
        UdpTrace(result.second == Errno::SUCCESS ? "sendto" : "sendto-failed", fd, addr_in,
                 result.first, message);
    }
    return result;
}

Errno BSD_USA::CloseImpl(s32 fd) {
    if (!IsFileDescriptorValid(fd)) {
        return Errno::BADF;
    }
    if (!file_descriptors[fd]->socket) {
        LOG_WARNING(Service, "Uninitialized socket");
        return Errno::BADF;
    }

    const Errno bsd_errno = Translate(file_descriptors[fd]->socket->Close());
    if (bsd_errno != Errno::SUCCESS) {
        return bsd_errno;
    }

    LOG_INFO(Service, "Close socket fd={}", fd);

    file_descriptors[fd].reset();
    return bsd_errno;
}

std::variant<s32, Errno> BSD_USA::DuplicateSocketImpl(s32 fd) {
    if (!IsFileDescriptorValid(fd)) {
        return Errno::BADF;
    }

    const s32 new_fd = FindFreeFileDescriptorHandle();
    if (new_fd < 0) {
        LOG_ERROR(Service, "No more file descriptors available");
        return Errno::MFILE;
    }

    file_descriptors[new_fd] = FileDescriptor{
        .socket = file_descriptors[fd]->socket,
        .flags = file_descriptors[fd]->flags,
        .is_connection_based = file_descriptors[fd]->is_connection_based,
    };
    return new_fd;
}

std::optional<std::shared_ptr<Network::SocketBase>> BSD_USA::GetSocket(s32 fd) {
    if (!IsFileDescriptorValid(fd)) {
        return std::nullopt;
    }
    if (!file_descriptors[fd]->socket) {
        LOG_WARNING(Service, "Uninitialized socket");
        return std::nullopt;
    }
    return file_descriptors[fd]->socket;
}

s32 BSD_USA::FindFreeFileDescriptorHandle() noexcept {
    for (s32 fd = 0; fd < static_cast<s32>(file_descriptors.size()); ++fd) {
        if (!file_descriptors[fd]) {
            return fd;
        }
    }
    return -1;
}

bool BSD_USA::IsFileDescriptorValid(s32 fd) const noexcept {
    if (fd > static_cast<s32>(MAX_FD) || fd < 0) {
        LOG_ERROR(Service, "Invalid file descriptor handle={}", fd);
        return false;
    }
    if (!file_descriptors[fd]) {
        LOG_ERROR(Service, "File descriptor handle={} is not allocated", fd);
        return false;
    }
    return true;
}

void BSD_USA::BuildErrnoResponse(HLERequestContext& ctx, Errno bsd_errno) const noexcept {
    IPC::ResponseBuilder rb{ctx, 4};

    rb.Push(ResultSuccess);
    rb.Push<s32>(bsd_errno == Errno::SUCCESS ? 0 : -1);
    rb.PushEnum(bsd_errno);
}

void BSD_USA::OnProxyPacketReceived(const Network::ProxyPacket& packet) {
    for (auto& optional_descriptor : file_descriptors) {
        if (!optional_descriptor.has_value()) {
            continue;
        }
        FileDescriptor& descriptor = *optional_descriptor;
        descriptor.socket.get()->HandleProxyPacket(packet);
    }
}

BSD_USA::BSD_USA(Core::System& system_, const char* name, bool is_user_)
    : ServiceFramework{system_, name}
    , is_user{is_user_} {
    // clang-format off
    static const FunctionInfo functions[] = {
        {0, &BSD_USA::RegisterClient, "RegisterClient"},
        {1, &BSD_USA::StartMonitoring, "StartMonitoring"},
        {2, &BSD_USA::Socket, "Socket"},
        {3, &BSD_USA::SocketExempt, "SocketExempt"},
        {4, nullptr, "Open"},
        {5, &BSD_USA::Select, "Select"},
        {6, &BSD_USA::Poll, "Poll"},
        {7, nullptr, "Sysctl"},
        {8, &BSD_USA::Recv, "Recv"},
        {9, &BSD_USA::RecvFrom, "RecvFrom"},
        {10, &BSD_USA::Send, "Send"},
        {11, &BSD_USA::SendTo, "SendTo"},
        {12, &BSD_USA::Accept, "Accept"},
        {13, &BSD_USA::Bind, "Bind"},
        {14, &BSD_USA::Connect, "Connect"},
        {15, &BSD_USA::GetPeerName, "GetPeerName"},
        {16, &BSD_USA::GetSockName, "GetSockName"},
        {17, &BSD_USA::GetSockOpt, "GetSockOpt"},
        {18, &BSD_USA::Listen, "Listen"},
        {19, nullptr, "Ioctl"},
        {20, &BSD_USA::Fcntl, "Fcntl"},
        {21, &BSD_USA::SetSockOpt, "SetSockOpt"},
        {22, &BSD_USA::Shutdown, "Shutdown"},
        {23, nullptr, "ShutdownAllSockets"},
        {24, &BSD_USA::Write, "Write"},
        {25, &BSD_USA::Read, "Read"},
        {26, &BSD_USA::Close, "Close"},
        {27, &BSD_USA::DuplicateSocket, "DuplicateSocket"},
        {28, &BSD_USA::StubReplyZeroed, "GetResourceStatistics"},
        {29, &BSD_USA::RecvMMsg, "RecvMMsg"}, //3.0.0+
        {30, &BSD_USA::SendMMsg, "SendMMsg"}, //3.0.0+
        {31, &BSD_USA::EventFd, "EventFd"}, //7.0.0+
        {32, &BSD_USA::StubReplyZeroed, "RegisterResourceStatisticsName"}, //7.0.0+
        {33, &BSD_USA::StubReplyZeroed, "RegisterClientShared"}, //10.0.0+
        {34, &BSD_USA::StubReplyZeroed, "GetSocketStatistics"}, //15.0.0+
        {35, &BSD_USA::StubReplyZeroed, "NifIoctl"}, //17.0.0+
        {36, &BSD_USA::RegisterClientWithVersion, "RegisterClientWithVersion"}, //18.0.0+
        {37, &BSD_USA::StubReplyZeroed, "Unknown37"}, //18.0.0+
        {38, &BSD_USA::StubReplyZeroed, "Unknown38"}, //18.0.0+
        {39, &BSD_USA::StubReplyZeroed, "Unknown39"}, //20.0.0+
        {40, &BSD_USA::StubReplyZeroed, "Unknown40"}, //20.0.0+
        {41, &BSD_USA::StubReplyEvent, "Unknown41"}, //21.0.0+
        {42, &BSD_USA::StubReplyZeroed, "Unknown42"}, //21.0.0+
        {43, &BSD_USA::StubReplyZeroed, "Unknown43"}, //21.0.0+
        {44, &BSD_USA::StubReplyZeroed, "Unknown44"},
        {200, &BSD_USA::StubReplyZeroed, "SetThreadCoreMask"}, //15.0.0+
        {201, &BSD_USA::StubReplyZeroed, "GetThreadCoreMask"}, //15.0.0+
    };
    // clang-format on

    RegisterHandlers(functions);

    if (auto room_member = Network::GetRoomMember().lock()) {
        proxy_packet_received = room_member->BindOnProxyPacketReceived(
            [this](const Network::ProxyPacket& packet) { OnProxyPacketReceived(packet); });
    } else {
        LOG_ERROR(Service, "Network isn't initialized");
    }
}

BSD_USA::~BSD_USA() {
    if (auto room_member = Network::GetRoomMember().lock()) {
        room_member->Unbind(proxy_packet_received);
    }
}

std::unique_lock<std::mutex> BSD_USA::LockService() noexcept {
    return {};
}

BSDCFG::BSDCFG(Core::System& system_, const char *name)
    : ServiceFramework{system_, name} {
    // clang-format off
    static const FunctionInfo functions[] = {
        {0, nullptr, "SetIfUp"},
        {1, nullptr, "SetIfUpWithEvent"},
        {2, nullptr, "CancelIf"},
        {3, nullptr, "SetIfDown"},
        {4, nullptr, "GetIfState"},
        {5, nullptr, "DhcpRenew"},
        {6, nullptr, "AddStaticArpEntry"},
        {7, nullptr, "RemoveArpEntry"},
        {8, nullptr, "LookupArpEntry"},
        {9, nullptr, "LookupArpEntry2"},
        {10, nullptr, "ClearArpEntries"},
        {11, nullptr, "ClearArpEntries2"},
        {12, nullptr, "PrintArpEntries"},
        {13, nullptr, "Unknown13"},
        {14, nullptr, "Unknown14"},
        {15, nullptr, "Unknown15"},
    };
    // clang-format on

    RegisterHandlers(functions);
}

BSDCFG::~BSDCFG() = default;

BSD_NU::BSD_NU(Core::System& system_)
    : ServiceFramework{system_, "bsd:nu"} {
    // clang-format off
    static const FunctionInfo functions[] = {
        {0, nullptr, "CreateUserService"},
    };
    // clang-format on
    RegisterHandlers(functions);
}

BSD_NU::~BSD_NU() = default;

} // namespace Service::Sockets
