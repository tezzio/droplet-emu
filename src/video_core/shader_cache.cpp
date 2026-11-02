// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <string>
#include <set>
#include <unordered_set>
#include <fmt/ranges.h>
#include <array>
#include <vector>
#include <boost/container/small_vector.hpp>

#include "common/assert.h"
#include "shader_recompiler/frontend/maxwell/control_flow.h"
#include "shader_recompiler/exception.h"
#include "shader_recompiler/object_pool.h"
#include "video_core/control/channel_state.h"
#include "video_core/dirty_flags.h"
#include "video_core/engines/kepler_compute.h"
#include "video_core/engines/maxwell_3d.h"
#include "video_core/host1x/gpu_device_memory_manager.h"
#include "video_core/memory_manager.h"
#include "video_core/shader_cache.h"
#include "video_core/shader_environment.h"
#include "video_core/texture_cache/util.h"

namespace VideoCommon {

void ShaderCache::InvalidateRegion(VAddr addr, size_t size) {
    std::scoped_lock lock{invalidation_mutex};
    InvalidatePagesInRegion(addr, size);
    RemovePendingShaders();
}

void ShaderCache::OnCacheInvalidation(VAddr addr, size_t size) {
    std::scoped_lock lock{invalidation_mutex};
    InvalidatePagesInRegion(addr, size);
}

void ShaderCache::SyncGuestHost() {
    std::scoped_lock lock{invalidation_mutex};
    RemovePendingShaders();
}

ShaderCache::ShaderCache(Tegra::MaxwellDeviceMemoryManager& device_memory_)
    : device_memory{device_memory_} {}

namespace {
// EDEN_NX2_MESH_TRACE: once per set of bound programs that includes a rejected native one, every
// pipeline slot (control byte and program) and the draw that uses them, to tell how task and
// mesh programs are launched. When it names a folder, the whole 3D register file is saved there
// too (mesh-<n>.bin), and again at the next draw that works (after-<n>.bin), to diff the state.
const char* const mesh_trace = std::getenv("EDEN_NX2_MESH_TRACE");
int mesh_dumps = 0;
bool mesh_after_pending = false;

void DumpRegisters(const Tegra::Engines::Maxwell3D& maxwell3d, const char* kind) {
    if (mesh_trace == nullptr || mesh_trace[0] != '/') {
        return;
    }
    const auto path = fmt::format("{}/{}-{}.bin", mesh_trace, kind, mesh_dumps);
    if (std::FILE* file = std::fopen(path.c_str(), "wb")) {
        std::fwrite(maxwell3d.regs.reg_array.data(), sizeof(u32), maxwell3d.regs.reg_array.size(), file);
        std::fclose(file);
    }
}

void TraceRejectedDraw(Tegra::Engines::Maxwell3D& maxwell3d) {
    if (mesh_trace == nullptr) {
        return;
    }
    static std::set<std::array<u64, 6>> seen;
    std::array<u64, 6> programs{};
    std::string slots;
    for (size_t i = 0; i < programs.size(); ++i) {
        const auto& config = maxwell3d.regs.pipelines[i];
        u32 control{};
        std::memcpy(&control, &config, sizeof(control));
        programs[i] = (u64{config.reserved[0] & 0xffU} << 32) | config.reserved[1];
        slots += fmt::format(" {}:{:x}/{:x}", i, control & 0xff, programs[i]);
    }
    if (!seen.insert(programs).second) {
        return;
    }
    const auto& draw = maxwell3d.draw_manager.draw_state;
    const auto& indirect = maxwell3d.draw_manager.indirect_state;
    LOG_INFO(HW_GPU,
             "Mesh trace:{} | topology {} mode {} indexed {} vertices {}+{} indices {}+{} "
             "instances {}+{} | indirect {:#x} count {:#x} draws {} stride {} indexed {} counted {}",
             slots, static_cast<u32>(draw.topology), static_cast<u32>(draw.draw_mode),
             draw.draw_indexed, draw.vertex_buffer.first, draw.vertex_buffer.count,
             draw.index_buffer.first, draw.index_buffer.count, draw.base_instance,
             draw.instance_count, indirect.indirect_start_address, indirect.count_start_address,
             indirect.max_draw_counts, indirect.stride, indirect.is_indexed, indirect.include_count);
    ++mesh_dumps;
    DumpRegisters(maxwell3d, "mesh");
    mesh_after_pending = true;
}

// Also once per set of task/mesh/pixel programs: their whole 128-byte native headers (Eden reads
// the first 0x50), to find where the per-primitive outputs are mapped.
void TraceMeshHeaders(const Tegra::Engines::Maxwell3D& maxwell3d, Tegra::MemoryManager& gpu_memory) {
    static std::set<std::array<u64, 3>> seen;
    static constexpr std::array<size_t, 3> slots{1, 3, 5};
    std::array<u64, 3> programs{};
    for (size_t i = 0; i < slots.size(); ++i) {
        if (maxwell3d.regs.IsShaderConfigEnabled(slots[i])) {
            const auto& config = maxwell3d.regs.pipelines[slots[i]];
            programs[i] = (u64{config.reserved[0] & 0xffU} << 32) | config.reserved[1];
        }
    }
    if (!seen.insert(programs).second) {
        return;
    }
    for (size_t i = 0; i < slots.size(); ++i) {
        if (programs[i] != 0) {
            std::array<u32, 32> header{};
            gpu_memory.ReadBlockUnsafe(programs[i], header.data(), sizeof(header));
            LOG_INFO(HW_GPU, "Mesh header: slot {} program {:x}: {:08x}", slots[i], programs[i],
                     fmt::join(header, " "));
        }
    }
}
} // Anonymous namespace

bool ShaderCache::RefreshStages(std::array<u64, 6>& unique_hashes) try {
    auto& dirty{maxwell3d->dirty.flags};
    if (!dirty[VideoCommon::Dirty::Shaders]) {
        return last_shaders_valid;
    }
    dirty[VideoCommon::Dirty::Shaders] = false;

    const GPUVAddr base_addr{maxwell3d->regs.program_region.Address()};
    const bool native = maxwell3d->GetNativeMacroEngine().native_turing;
    for (size_t index = 0; index < Tegra::Engines::Maxwell3D::Regs::MaxShaderProgram; ++index) {
        if (!maxwell3d->regs.IsShaderConfigEnabled(index)) {
            unique_hashes[index] = 0;
            continue;
        }
        // A task or mesh program in slot 1 comes with a header-only passthrough geometry program
        // in slot 4, which the host pipeline leaves out (docs/MESH_SHADING.md)
        if (native && index == 4 && shader_infos[1] != nullptr && shader_infos[1]->uses_stage_buffer) {
            unique_hashes[index] = 0;
            continue;
        }
        const auto& shader_config{maxwell3d->regs.pipelines[index]};
        const auto program{static_cast<Tegra::Engines::Maxwell3D::Regs::ShaderType>(index)};
        if (program == Tegra::Engines::Maxwell3D::Regs::ShaderType::Pixel &&
            !maxwell3d->regs.rasterize_enable) {
            unique_hashes[index] = 0;
            continue;
        }
        const GPUVAddr shader_addr{native
            ? (static_cast<u64>(shader_config.reserved[0] & 0xff) << 32) | shader_config.reserved[1]
            : base_addr + shader_config.offset};
        const std::optional<VAddr> cpu_shader_addr{gpu_memory->GpuToCpuAddress(shader_addr)};
        if (!cpu_shader_addr) {
            LOG_ERROR(HW_GPU, "Invalid GPU address for shader {:#016x}", shader_addr);
            last_shaders_valid = false;
            return false;
        }
        const ShaderInfo* shader_info{TryGet(*cpu_shader_addr)};
        if (!shader_info && native && IsRejected(*cpu_shader_addr)) {
            // A rejected program fails the same way every time it is bound: skip its draws
            // without rethrowing (two C++ throws each time cost ~1-2% of the GPU thread).
            TraceRejectedDraw(*maxwell3d);
            maxwell3d->GetNativeMacroEngine().native_execution_failed = true;
            last_shaders_valid = false;
            return false;
        }
        if (!shader_info) {
            const u32 start_address{native ? 0 : shader_config.offset};
            try {
                // Native programs are lowered while the environment is built.
                RethrowIfRejected(*cpu_shader_addr);
                GraphicsEnvironment env{*maxwell3d, *gpu_memory, program, native ? shader_addr : base_addr, start_address};
                shader_info = MakeShaderInfo(env, *cpu_shader_addr);
            } catch (const Shader::Exception&) {
                RememberRejection(*cpu_shader_addr);
                // Once per program: which stage slot it is bound to, its header and the slot's
                // registers (to tell how a pipeline with task/mesh programs is set up).
                static std::unordered_set<GPUVAddr> described;
                if (native && described.insert(shader_addr).second) {
                    std::array<u32, 5> header{};
                    gpu_memory->ReadBlockUnsafe(shader_addr, header.data(), sizeof(header));
                    std::array<u32, sizeof(shader_config) / sizeof(u32)> config{};
                    std::memcpy(config.data(), &shader_config, sizeof(config));
                    LOG_ERROR(HW_GPU, "Rejected program {:#x} is bound to stage {}: header {:08x}, slot {:08x}",
                              shader_addr, index, fmt::join(header, " "), fmt::join(config, " "));
                }
                TraceRejectedDraw(*maxwell3d);
                throw;
            }
        }
        shader_infos[index] = shader_info;
        unique_hashes[index] = shader_info->unique_hash;
    }
    if (mesh_trace != nullptr && native && shader_infos[1] != nullptr &&
        shader_infos[1]->uses_stage_buffer) {
        TraceMeshHeaders(*maxwell3d, *gpu_memory);
    }
    if (mesh_after_pending && native) {
        DumpRegisters(*maxwell3d, "after");
        mesh_after_pending = false;
    }
    last_shaders_valid = true;
    return true;
} catch (const Shader::Exception& error) {
    auto& native = maxwell3d->GetNativeMacroEngine();
    if (!native.native_turing) throw;
    native.native_execution_failed = true;
    last_shaders_valid = false;
    // Each rejected program is logged once (the draws that use it keep failing quietly).
    static std::unordered_set<std::string> logged;
    if (logged.insert(error.what()).second) {
        LOG_ERROR(HW_GPU, "Native graphics shader rejected: {}", error.what());
    }
    // Once: show what the vertex stage has bound, to find where an unbound bank's data lives.
    static std::vector<std::string> dumped_errors;
    if (const std::string what{error.what()};
        what.find("constant buffer") != std::string::npos && dumped_errors.size() < 12 &&
        std::count(dumped_errors.begin(), dumped_errors.end(), what) < 3) {
        dumped_errors.push_back(what);
        LOG_INFO(HW_GPU, "VS cbuf dump for: {}", what.substr(0, 40));
        for (u32 slot = 3; slot < 8; ++slot) {
            const auto& buffer = maxwell3d->state.shader_stages[0].const_buffers[slot];
            if (!buffer.enabled) continue;
            std::array<u32, 24> words{};
            gpu_memory->ReadBlockUnsafe(buffer.address, words.data(), sizeof(words));
            LOG_INFO(HW_GPU, "VS cbuf {} at {:#x} size {:#x}: {:08x}", slot, buffer.address, buffer.size,
                     fmt::join(words, " "));
        }
    }
    return false;
}

const ShaderInfo* ShaderCache::ComputeShader() {
    const GPUVAddr program_base{kepler_compute->regs.code_loc.Address()};
    const auto& qmd{kepler_compute->launch_description};
    const GPUVAddr shader_addr{program_base + qmd.program_start};
    const std::optional<VAddr> cpu_shader_addr{gpu_memory->GpuToCpuAddress(shader_addr)};
    if (!cpu_shader_addr) {
        LOG_ERROR(HW_GPU, "Invalid GPU address for shader {:#016x}", shader_addr);
        return nullptr;
    }
    if (const ShaderInfo* const shader = TryGet(*cpu_shader_addr)) {
        return shader;
    }
    try {
        RethrowIfRejected(*cpu_shader_addr);
        ComputeEnvironment env{*kepler_compute, *gpu_memory, program_base, qmd.program_start};
        return MakeShaderInfo(env, *cpu_shader_addr);
    } catch (const Shader::Exception&) {
        RememberRejection(*cpu_shader_addr);
        throw;
    }
}

void ShaderCache::RethrowIfRejected(VAddr addr) {
    std::scoped_lock lock{rejected_mutex};
    if (const auto it = rejected_native.find(addr); it != rejected_native.end()) {
        std::rethrow_exception(it->second);
    }
}

bool ShaderCache::IsRejected(VAddr addr) {
    std::scoped_lock lock{rejected_mutex};
    return rejected_native.contains(addr);
}

void ShaderCache::RememberRejection(VAddr addr) {
    std::scoped_lock lock{rejected_mutex};
    rejected_native.emplace(addr, std::current_exception());
    rejected_lowest.store((std::min)(rejected_lowest.load(std::memory_order_relaxed), addr),
                          std::memory_order_relaxed);
    rejected_highest.store((std::max)(rejected_highest.load(std::memory_order_relaxed), addr),
                           std::memory_order_relaxed);
}

void ShaderCache::GetGraphicsEnvironments(GraphicsEnvironments& result,
                                          const std::array<u64, NUM_PROGRAMS>& unique_hashes) {
    size_t env_index{};
    const GPUVAddr base_addr{maxwell3d->regs.program_region.Address()};
    const bool native = maxwell3d->GetNativeMacroEngine().native_turing;
    for (size_t index = 0; index < NUM_PROGRAMS; ++index) {
        if (unique_hashes[index] == 0) {
            continue;
        }
        const auto program{static_cast<Tegra::Engines::Maxwell3D::Regs::ShaderType>(index)};
        auto& env{result.envs[index]};
        const auto& config = maxwell3d->regs.pipelines[index];
        const u32 start_address{native ? 0 : config.offset};
        const GPUVAddr address = native
            ? (static_cast<u64>(config.reserved[0] & 0xff) << 32) | config.reserved[1]
            : base_addr;
        env = GraphicsEnvironment{*maxwell3d, *gpu_memory, program, address, start_address};
        env.SetCachedSize(shader_infos[index]->size_bytes);
        result.env_ptrs[env_index++] = &env;
    }
}

ShaderInfo* ShaderCache::TryGet(VAddr addr) const {
    std::scoped_lock lock{lookup_mutex};

    const auto it = lookup_cache.find(addr);
    if (it == lookup_cache.end()) {
        return nullptr;
    }
    return it->second->data;
}

void ShaderCache::Register(std::unique_ptr<ShaderInfo> data, VAddr addr, size_t size) {
    std::scoped_lock lock{invalidation_mutex, lookup_mutex};

    const VAddr addr_end = addr + size;
    Entry* const entry = NewEntry(addr, addr_end, data.get());

    const u64 page_end = (addr_end + YUZU_PAGESIZE - 1) >> YUZU_PAGEBITS;
    for (u64 page = addr >> YUZU_PAGEBITS; page < page_end; ++page) {
        invalidation_cache[page].push_back(entry);
    }

    storage.push_back(std::move(data));

    device_memory.UpdatePagesCachedCount(addr, size, 1);
}

void ShaderCache::InvalidatePagesInRegion(VAddr addr, size_t size) {
    const VAddr addr_end = addr + size;
    // Rejected programs are few and in a few places: look at them only for writes that can
    // reach them (this runs for every inline upload and pre-draw invalidation).
    if (addr_end > rejected_lowest.load(std::memory_order_relaxed) &&
        addr <= rejected_highest.load(std::memory_order_relaxed)) {
        std::scoped_lock lock{rejected_mutex};
        boost::unordered::erase_if(rejected_native, [&](const auto& rejected) {
            return rejected.first >= addr && rejected.first < addr_end;
        });
    }
    const u64 page_end = (addr_end + YUZU_PAGESIZE - 1) >> YUZU_PAGEBITS;
    for (u64 page = addr >> YUZU_PAGEBITS; page < page_end; ++page) {
        auto it = invalidation_cache.find(page);
        if (it == invalidation_cache.end()) {
            continue;
        }
        InvalidatePageEntries(it->second, addr, addr_end);
    }
}

void ShaderCache::RemovePendingShaders() {
    if (marked_for_removal.empty()) {
        return;
    }
    // Remove duplicates
    std::ranges::sort(marked_for_removal);
    marked_for_removal.erase(std::unique(marked_for_removal.begin(), marked_for_removal.end()),
                             marked_for_removal.end());
    // Linear growth anyways - maybe consider static_vector instead?
    boost::container::small_vector<ShaderInfo*, 16> removed_shaders;
    std::scoped_lock lock{lookup_mutex};
    for (Entry* const entry : marked_for_removal) {
        removed_shaders.push_back(entry->data);
        auto const it = lookup_cache.find(entry->addr_start);
        ASSERT(it != lookup_cache.end());
        lookup_cache.erase(it);
    }
    marked_for_removal.clear();

    if (!removed_shaders.empty()) {
        // Remove the given shaders from the cache
        std::erase_if(storage, [&removed_shaders](const std::unique_ptr<ShaderInfo>& shader) {
            return std::ranges::find(removed_shaders, shader.get()) != removed_shaders.end();
        });
    }
}

void ShaderCache::InvalidatePageEntries(std::vector<Entry*>& entries, VAddr addr, VAddr addr_end) {
    size_t index = 0;
    while (index < entries.size()) {
        Entry* const entry = entries[index];
        if (!entry->Overlaps(addr, addr_end)) {
            ++index;
            continue;
        }

        UnmarkMemory(entry);
        RemoveEntryFromInvalidationCache(entry);
        marked_for_removal.push_back(entry);
    }
}

void ShaderCache::RemoveEntryFromInvalidationCache(const Entry* entry) {
    const u64 page_end = (entry->addr_end + YUZU_PAGESIZE - 1) >> YUZU_PAGEBITS;
    for (u64 page = entry->addr_start >> YUZU_PAGEBITS; page < page_end; ++page) {
        const auto entries_it = invalidation_cache.find(page);
        ASSERT(entries_it != invalidation_cache.end());
        std::vector<Entry*>& entries = entries_it->second;

        const auto entry_it = std::ranges::find(entries, entry);
        ASSERT(entry_it != entries.end());
        entries.erase(entry_it);
    }
}

void ShaderCache::UnmarkMemory(Entry* entry) {
    if (!entry->is_memory_marked) {
        return;
    }
    entry->is_memory_marked = false;

    const VAddr addr = entry->addr_start;
    const size_t size = entry->addr_end - addr;
    device_memory.UpdatePagesCachedCount(addr, size, -1);
}

ShaderCache::Entry* ShaderCache::NewEntry(VAddr addr, VAddr addr_end, ShaderInfo* data) {
    auto entry = std::make_unique<Entry>(Entry{addr, addr_end, data});
    Entry* const entry_pointer = entry.get();

    lookup_cache.emplace(addr, std::move(entry));
    return entry_pointer;
}

const ShaderInfo* ShaderCache::MakeShaderInfo(GenericEnvironment& env, VAddr cpu_addr) {
    auto info = std::make_unique<ShaderInfo>();
    if (const std::optional<u64> cached_hash{env.Analyze()}) {
        info->unique_hash = *cached_hash;
        info->size_bytes = env.CachedSizeBytes();
    } else {
        // Slow path, not really hit on commercial games
        // Build a control flow graph to get the real shader size
        Shader::ObjectPool<Shader::Maxwell::Flow::Block> flow_block;
        Shader::Maxwell::Flow::CFG cfg{env, flow_block, env.StartAddress()};
        info->unique_hash = env.CalculateHash();
        info->size_bytes = env.ReadSizeBytes();
    }
    info->uses_stage_buffer = env.UsesStageBuffer();
    const size_t size_bytes{env.SourceSizeBytes() ? env.SourceSizeBytes() : info->size_bytes};
    const ShaderInfo* const result{info.get()};
    Register(std::move(info), cpu_addr, size_bytes);
    return result;
}

} // namespace VideoCommon
