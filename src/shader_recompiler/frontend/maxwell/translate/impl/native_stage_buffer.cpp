// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "common/bit_field.h"
#include "common/common_types.h"
#include "shader_recompiler/frontend/maxwell/translate/impl/impl.h"

namespace Shader::Maxwell {
namespace {
// Pseudo instructions from the Ampere lowering (ISBERD/ISBEWR, docs/MESH_SHADING.md): register
// (destination or value) in 0-7, address register in 8-15, offset in 20-35, StageBufferFlags in 36-38.
union StageBufferAccess {
    u64 raw;
    BitField<0, 8, IR::Reg> reg;
    BitField<8, 8, IR::Reg> address_reg;
    BitField<20, 16, u64> offset;
    BitField<36, 3, u64> flags;
};

IR::U32 Address(IR::IREmitter& ir, TranslatorVisitor& v, const StageBufferAccess& access) {
    const IR::U32 offset{ir.Imm32(static_cast<u32>(access.offset.Value()))};
    return access.address_reg == IR::Reg::RZ ? offset : IR::U32{ir.IAdd(v.X(access.address_reg), offset)};
}
} // Anonymous namespace

void TranslatorVisitor::NX2_ISBERD(u64 insn) {
    const StageBufferAccess access{insn};
    X(access.reg, ir.ReadStageBuffer(Address(ir, *this, access), static_cast<u32>(access.flags.Value())));
}

void TranslatorVisitor::NX2_ISBEWR(u64 insn) {
    const StageBufferAccess access{insn};
    ir.WriteStageBuffer(Address(ir, *this, access), X(access.reg), static_cast<u32>(access.flags.Value()));
}

} // namespace Shader::Maxwell
