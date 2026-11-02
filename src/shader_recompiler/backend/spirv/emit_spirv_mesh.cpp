// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "shader_recompiler/backend/spirv/emit_spirv.h"
#include "shader_recompiler/backend/spirv/emit_spirv_instructions.h"
#include "shader_recompiler/backend/spirv/spirv_emit_context.h"

namespace Shader::Backend::SPIRV {
namespace {
// Indexes past the end of an array land in its spare last element
Id Clamp(EmitContext& ctx, Id index, u32 last) {
    return ctx.OpUMin(ctx.U32[1], index, ctx.Const(last));
}

Id Word(EmitContext& ctx, Id byte_address, u32 last) {
    return Clamp(ctx, ctx.OpShiftRightLogical(ctx.U32[1], byte_address, ctx.Const(2U)), last);
}
} // Anonymous namespace

// The stage buffer of Switch 2 task and mesh programs (docs/MESH_SHADING.md)
Id EmitReadStageBuffer(EmitContext& ctx, const IR::Value& address, const IR::Value&) {
    // Word 0: the workgroup's index, or a mesh workgroup's index within its task's launch
    if (address.IsImmediate() && address.U32() == 0) {
        return ctx.OpCompositeExtract(ctx.U32[1], ctx.OpLoad(ctx.U32[3], ctx.mesh_workgroup_id), 0U);
    }
    if (!Sirit::ValidId(ctx.task_payload)) {
        return ctx.u32_zero_value;
    }
    const Id word{Word(ctx, ctx.Def(address), EmitContext::TASK_PAYLOAD_WORDS - 1)};
    return ctx.OpLoad(ctx.U32[1], ctx.OpAccessChain(ctx.task_payload_pointer, ctx.task_payload, word));
}

void EmitWriteStageBuffer(EmitContext& ctx, const IR::Value& address, Id value,
                          const IR::Value& flags) {
    const u32 mode{flags.U32()};
    const Id byte_address{ctx.Def(address)};
    if (ctx.stage == Stage::Task) {
        // The payload, and in word 1 how many mesh workgroups to launch
        if ((mode & StageBufferAttr) != 0) {
            const Id word{Word(ctx, byte_address, EmitContext::TASK_PAYLOAD_WORDS - 1)};
            ctx.OpStore(ctx.OpAccessChain(ctx.task_payload_pointer, ctx.task_payload, word), value);
        }
        return;
    }
    if (ctx.stage != Stage::Mesh) {
        return;
    }
    if ((mode & StageBufferAttr) != 0) {
        const Id word{Word(ctx, byte_address, ctx.mesh_attribute_words)};
        ctx.OpStore(ctx.OpAccessChain(ctx.mesh_workgroup_pointer, ctx.mesh_attributes, word), value);
    } else if ((mode & StageBufferWord) != 0) {
        // The only word in the primitive space: the primitive count (at byte 3)
        ctx.OpStore(ctx.mesh_primitive_count, value);
    } else {
        const Id index{Clamp(ctx, byte_address, ctx.mesh_index_byte_count)};
        ctx.OpStore(ctx.OpAccessChain(ctx.mesh_workgroup_pointer, ctx.mesh_index_bytes, index),
                    ctx.OpBitwiseAnd(ctx.U32[1], value, ctx.Const(0xffU)));
    }
}

} // namespace Shader::Backend::SPIRV
