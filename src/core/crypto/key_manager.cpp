// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <array>
#include <cstring>
#include <map>
#include <span>
#include <variant>

#include "common/assert.h"
#include "common/logging.h"
#include "core/crypto/key_manager.h"
#include "core/file_sys/vfs/vfs.h"

namespace Core::Crypto {

u64 GetSignatureTypeDataSize(SignatureType type) {
    switch (type) {
    case SignatureType::RSA_4096_SHA1:
    case SignatureType::RSA_4096_SHA256:
        return 0x200;
    case SignatureType::RSA_2048_SHA1:
    case SignatureType::RSA_2048_SHA256:
        return 0x100;
    case SignatureType::ECDSA_SHA1:
    case SignatureType::ECDSA_SHA256:
        return 0x3C;
    }
    UNREACHABLE();
}

u64 GetSignatureTypePaddingSize(SignatureType type) {
    switch (type) {
    case SignatureType::RSA_4096_SHA1:
    case SignatureType::RSA_4096_SHA256:
    case SignatureType::RSA_2048_SHA1:
    case SignatureType::RSA_2048_SHA256:
        return 0x3C;
    case SignatureType::ECDSA_SHA1:
    case SignatureType::ECDSA_SHA256:
        return 0x40;
    }
    UNREACHABLE();
}

bool Ticket::IsValid() const {
    return !std::holds_alternative<std::monostate>(data);
}

SignatureType Ticket::GetSignatureType() const {
    if (const auto* ticket = std::get_if<RSA4096Ticket>(&data))
        return ticket->sig_type;
    if (const auto* ticket = std::get_if<RSA2048Ticket>(&data))
        return ticket->sig_type;
    if (const auto* ticket = std::get_if<ECDSATicket>(&data))
        return ticket->sig_type;
    UNREACHABLE();
}

TicketData& Ticket::GetData() {
    if (auto* ticket = std::get_if<RSA4096Ticket>(&data))
        return ticket->data;
    if (auto* ticket = std::get_if<RSA2048Ticket>(&data))
        return ticket->data;
    if (auto* ticket = std::get_if<ECDSATicket>(&data))
        return ticket->data;
    UNREACHABLE();
}

const TicketData& Ticket::GetData() const {
    if (const auto* ticket = std::get_if<RSA4096Ticket>(&data))
        return ticket->data;
    if (const auto* ticket = std::get_if<RSA2048Ticket>(&data))
        return ticket->data;
    if (const auto* ticket = std::get_if<ECDSATicket>(&data))
        return ticket->data;
    UNREACHABLE();
}

u64 Ticket::GetSize() const {
    const auto sig_type = GetSignatureType();

    return sizeof(SignatureType) + GetSignatureTypeDataSize(sig_type) + GetSignatureTypePaddingSize(sig_type) + sizeof(TicketData);
}

Ticket Ticket::SynthesizeCommon(Key128 title_key, const std::array<u8, 16>& rights_id) {
    RSA2048Ticket out{};
    out.sig_type = SignatureType::RSA_2048_SHA256;
    out.data.rights_id = rights_id;
    out.data.title_key_common = title_key;
    return Ticket{out};
}

Ticket Ticket::Read(const FileSys::VirtualFile& file) {
    // Attempt to read up to the largest ticket size, and make sure we read at least a signature
    // type.
    std::array<u8, sizeof(RSA4096Ticket)> raw_data{};
    auto read_size = file->Read(raw_data.data(), raw_data.size(), 0);
    if (read_size < sizeof(SignatureType)) {
        LOG_WARNING(Crypto, "Attempted to read ticket file with invalid size {}.", read_size);
        return Ticket{std::monostate()};
    }
    return Read(std::span{raw_data});
}

Ticket Ticket::Read(std::span<const u8> raw_data) {
    // Some tools read only 0x180 bytes of ticket data instead of 0x2C0, so
    // just make sure we have at least the bare minimum of data to work with.
    SignatureType sig_type;
    if (raw_data.size() < sizeof(SignatureType)) {
        LOG_WARNING(Crypto, "Attempted to parse ticket buffer with invalid size {}.", raw_data.size());
        return Ticket{std::monostate()};
    }
    std::memcpy(&sig_type, raw_data.data(), sizeof(sig_type));

    switch (sig_type) {
    case SignatureType::RSA_4096_SHA1:
    case SignatureType::RSA_4096_SHA256: {
        RSA4096Ticket ticket{};
        std::memcpy(&ticket, raw_data.data(), sizeof(ticket));
        return Ticket{ticket};
    }
    case SignatureType::RSA_2048_SHA1:
    case SignatureType::RSA_2048_SHA256: {
        RSA2048Ticket ticket{};
        std::memcpy(&ticket, raw_data.data(), sizeof(ticket));
        return Ticket{ticket};
    }
    case SignatureType::ECDSA_SHA1:
    case SignatureType::ECDSA_SHA256: {
        ECDSATicket ticket{};
        std::memcpy(&ticket, raw_data.data(), sizeof(ticket));
        return Ticket{ticket};
    }
    default:
        LOG_WARNING(Crypto, "Attempted to parse ticket buffer with invalid type {}.", sig_type);
        return Ticket{std::monostate()};
    }
}

// Droplet: the key manager has no key source. It reads no key files, derives nothing from console
// dumps and imports no tickets, so it can't decrypt Switch content. Switch 2 titles run from
// decrypted ExeFS/RomFS dumps and need no keys.
KeyManager::KeyManager() = default;

bool KeyManager::HasKey(S128KeyType id, u64 field1, u64 field2) const {
    return s128_keys.find({id, field1, field2}) != s128_keys.end();
}

bool KeyManager::HasKey(S256KeyType id, u64 field1, u64 field2) const {
    return s256_keys.find({id, field1, field2}) != s256_keys.end();
}

Key128 KeyManager::GetKey(S128KeyType id, u64 field1, u64 field2) const {
    if (!HasKey(id, field1, field2)) {
        return {};
    }
    return s128_keys.at({id, field1, field2});
}

Key256 KeyManager::GetKey(S256KeyType id, u64 field1, u64 field2) const {
    if (!HasKey(id, field1, field2)) {
        return {};
    }
    return s256_keys.at({id, field1, field2});
}

Key256 KeyManager::GetBISKey(u8 partition_id) const {
    Key256 out{};

    for (const auto& bis_type : {BISKeyType::Crypto, BISKeyType::Tweak}) {
        if (HasKey(S128KeyType::BIS, partition_id, u64(bis_type))) {
            std::memcpy(
                out.data() + sizeof(Key128) * u64(bis_type),
                s128_keys.at({S128KeyType::BIS, partition_id, u64(bis_type)}).data(),
                sizeof(Key128));
        }
    }

    return out;
}

const std::map<u128, Ticket>& KeyManager::GetCommonTickets() const {
    return common_tickets;
}

const std::map<u128, Ticket>& KeyManager::GetPersonalizedTickets() const {
    return personal_tickets;
}
} // namespace Core::Crypto
