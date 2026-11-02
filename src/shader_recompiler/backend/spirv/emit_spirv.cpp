// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <span>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "common/div_ceil.h"
#include "common/settings.h"
#include "shader_recompiler/backend/spirv/emit_spirv.h"
#include "shader_recompiler/backend/spirv/emit_spirv_instructions.h"
#include "shader_recompiler/backend/spirv/spirv_emit_context.h"
#include "shader_recompiler/frontend/ir/basic_block.h"
#include "shader_recompiler/frontend/ir/program.h"

namespace Shader::Backend::SPIRV {
namespace {
template <class Func>
struct FuncTraits {};

template <class ReturnType_, class... Args>
struct FuncTraits<ReturnType_ (*)(Args...)> {
    using ReturnType = ReturnType_;

    static constexpr size_t NUM_ARGS = sizeof...(Args);

    template <size_t I>
    using ArgType = std::tuple_element_t<I, std::tuple<Args...>>;
};

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4702) // Ignore unreachable code warning
#endif

template <auto func, typename... Args>
void SetDefinition(EmitContext& ctx, IR::Inst* inst, Args... args) {
    inst->SetDefinition<Id>(func(ctx, std::forward<Args>(args)...));
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

template <typename ArgType>
ArgType Arg(EmitContext& ctx, const IR::Value& arg) {
    if constexpr (std::is_same_v<ArgType, Id>) {
        return ctx.Def(arg);
    } else if constexpr (std::is_same_v<ArgType, const IR::Value&>) {
        return arg;
    } else if constexpr (std::is_same_v<ArgType, u32>) {
        return arg.U32();
    } else if constexpr (std::is_same_v<ArgType, IR::Attribute>) {
        return arg.Attribute();
    } else if constexpr (std::is_same_v<ArgType, IR::Patch>) {
        return arg.Patch();
    } else if constexpr (std::is_same_v<ArgType, IR::Reg>) {
        return arg.Reg();
    }
}

template <auto func, bool is_first_arg_inst, size_t... I>
void Invoke(EmitContext& ctx, IR::Inst* inst, std::index_sequence<I...>) {
    using Traits = FuncTraits<decltype(func)>;
    if constexpr (std::is_same_v<typename Traits::ReturnType, Id>) {
        if constexpr (is_first_arg_inst) {
            SetDefinition<func>(
                ctx, inst, inst,
                Arg<typename Traits::template ArgType<I + 2>>(ctx, inst->Arg(I))...);
        } else {
            SetDefinition<func>(
                ctx, inst, Arg<typename Traits::template ArgType<I + 1>>(ctx, inst->Arg(I))...);
        }
    } else {
        if constexpr (is_first_arg_inst) {
            func(ctx, inst, Arg<typename Traits::template ArgType<I + 2>>(ctx, inst->Arg(I))...);
        } else {
            func(ctx, Arg<typename Traits::template ArgType<I + 1>>(ctx, inst->Arg(I))...);
        }
    }
}

template <auto func>
void Invoke(EmitContext& ctx, IR::Inst* inst) {
    using Traits = FuncTraits<decltype(func)>;
    static_assert(Traits::NUM_ARGS >= 1, "Insufficient arguments");
    if constexpr (Traits::NUM_ARGS == 1) {
        Invoke<func, false>(ctx, inst, std::make_index_sequence<0>{});
    } else {
        using FirstArgType = typename Traits::template ArgType<1>;
        static constexpr bool is_first_arg_inst = std::is_same_v<FirstArgType, IR::Inst*>;
        using Indices = std::make_index_sequence<Traits::NUM_ARGS - (is_first_arg_inst ? 2 : 1)>;
        Invoke<func, is_first_arg_inst>(ctx, inst, Indices{});
    }
}

void EmitInst(EmitContext& ctx, IR::Inst* inst) {
    switch (inst->GetOpcode()) {
#define OPCODE(name, result_type, ...)                                                             \
    case IR::Opcode::name:                                                                         \
        return Invoke<&Emit##name>(ctx, inst);
#include "shader_recompiler/frontend/ir/opcodes.inc"
#undef OPCODE
    }
    throw LogicError("Invalid opcode {}", inst->GetOpcode());
}

Id TypeId(const EmitContext& ctx, IR::Type type) {
    switch (type) {
    case IR::Type::U1:
        return ctx.U1;
    case IR::Type::U32:
        return ctx.U32[1];
    default:
        throw NotImplementedException("Phi node type {}", type);
    }
}

void Traverse(EmitContext& ctx, IR::Program& program) {
    IR::Block* current_block{};
    for (const IR::AbstractSyntaxNode& node : program.syntax_list) {
        switch (node.type) {
        case IR::AbstractSyntaxNode::Type::Block: {
            const Id label{node.data.block->Definition<Id>()};
            if (current_block) {
                ctx.OpBranch(label);
            }
            current_block = node.data.block;
            ctx.AddLabel(label);
            for (IR::Inst& inst : node.data.block->Instructions()) {
                EmitInst(ctx, &inst);
            }
            break;
        }
        case IR::AbstractSyntaxNode::Type::If: {
            const Id if_label{node.data.if_node.body->Definition<Id>()};
            const Id endif_label{node.data.if_node.merge->Definition<Id>()};
            ctx.OpSelectionMerge(endif_label, spv::SelectionControlMask::MaskNone);
            ctx.OpBranchConditional(ctx.Def(node.data.if_node.cond), if_label, endif_label);
            break;
        }
        case IR::AbstractSyntaxNode::Type::Loop: {
            const Id body_label{node.data.loop.body->Definition<Id>()};
            const Id continue_label{node.data.loop.continue_block->Definition<Id>()};
            const Id endloop_label{node.data.loop.merge->Definition<Id>()};

            ctx.OpLoopMerge(endloop_label, continue_label, spv::LoopControlMask::MaskNone);
            ctx.OpBranch(body_label);
            break;
        }
        case IR::AbstractSyntaxNode::Type::Break: {
            const Id break_label{node.data.break_node.merge->Definition<Id>()};
            const Id skip_label{node.data.break_node.skip->Definition<Id>()};
            ctx.OpBranchConditional(ctx.Def(node.data.break_node.cond), break_label, skip_label);
            break;
        }
        case IR::AbstractSyntaxNode::Type::EndIf:
            if (current_block) {
                ctx.OpBranch(node.data.end_if.merge->Definition<Id>());
            }
            break;
        case IR::AbstractSyntaxNode::Type::Repeat: {
            Id cond{ctx.Def(node.data.repeat.cond)};
            if (!Settings::values.disable_shader_loop_safety_checks) {
                const Id pointer_type{ctx.TypePointer(spv::StorageClass::Private, ctx.U32[1])};
                const Id safety_counter{ctx.AddGlobalVariable(
                    pointer_type, spv::StorageClass::Private, ctx.Const(0x2000u))};
                if (ctx.profile.supported_spirv >= 0x00010400) {
                    ctx.interfaces.push_back(safety_counter);
                }
                const Id old_counter{ctx.OpLoad(ctx.U32[1], safety_counter)};
                const Id new_counter{ctx.OpISub(ctx.U32[1], old_counter, ctx.Const(1u))};
                ctx.OpStore(safety_counter, new_counter);

                const Id safety_cond{
                    ctx.OpSGreaterThanEqual(ctx.U1, new_counter, ctx.u32_zero_value)};
                cond = ctx.OpLogicalAnd(ctx.U1, cond, safety_cond);
            }
            const Id loop_header_label{node.data.repeat.loop_header->Definition<Id>()};
            const Id merge_label{node.data.repeat.merge->Definition<Id>()};
            ctx.OpBranchConditional(cond, loop_header_label, merge_label);
            break;
        }
        case IR::AbstractSyntaxNode::Type::Return:
            ctx.OpReturn();
            break;
        case IR::AbstractSyntaxNode::Type::Unreachable:
            ctx.OpUnreachable();
            break;
        }
        if (node.type != IR::AbstractSyntaxNode::Type::Block) {
            current_block = nullptr;
        }
    }
}

Id DefineMain(EmitContext& ctx, IR::Program& program) {
    const Id void_function{ctx.TypeFunction(ctx.void_id)};
    const Id main{ctx.OpFunction(ctx.void_id, spv::FunctionControlMask::MaskNone, void_function)};
    for (IR::Block* const block : program.blocks) {
        block->SetDefinition(ctx.OpLabel());
    }
    Traverse(ctx, program);
    ctx.OpFunctionEnd();
    return main;
}

// Task and mesh programs (docs/MESH_SHADING.md) run as a function of their own: an EXIT returns
// to main, so every invocation reaches the epilogue, which needs uniform control flow.
void MeshBarrier(EmitContext& ctx) {
    ctx.OpControlBarrier(ctx.Const(static_cast<u32>(spv::Scope::Workgroup)),
                         ctx.Const(static_cast<u32>(spv::Scope::Workgroup)),
                         ctx.Const(static_cast<u32>(spv::MemorySemanticsMask::AcquireRelease |
                                                    spv::MemorySemanticsMask::WorkgroupMemory)));
}

template <typename Body>
void IfThen(EmitContext& ctx, Id condition, Body&& body) {
    const Id then_label{ctx.OpLabel()};
    const Id merge_label{ctx.OpLabel()};
    ctx.OpSelectionMerge(merge_label, spv::SelectionControlMask::MaskNone);
    ctx.OpBranchConditional(condition, then_label, merge_label);
    ctx.AddLabel(then_label);
    body();
    ctx.OpBranch(merge_label);
    ctx.AddLabel(merge_label);
}

// The stage buffer's vertex blocks and index bytes, as the mesh shader's outputs
void EmitMeshOutputs(EmitContext& ctx, Id lane) {
    const RuntimeInfo& info{ctx.runtime_info};
    const u32 max_vertices{info.mesh_max_vertices};
    const u32 max_primitives{info.mesh_max_primitives};
    // EDEN_NX2_MESH_DEBUG: one full-screen triangle instead of the program's output, to check the
    // draw path on its own
    static const bool debug = std::getenv("EDEN_NX2_MESH_DEBUG") != nullptr;
    const Id count{debug ? ctx.Const(1U)
                         : ctx.OpUMin(ctx.U32[1], ctx.OpLoad(ctx.U32[1], ctx.mesh_primitive_count),
                                      ctx.Const(max_primitives))};
    const Id vertex_count{ctx.Const(max_vertices)};
    if (debug) {
        ctx.OpStore(vertex_count, count);
        ctx.set_mesh_outputs_stand_in = std::make_pair(vertex_count.value, count.value);
        const std::array<std::array<float, 2>, 3> corners{{{-1.0f, -1.0f}, {3.0f, -1.0f}, {-1.0f, 3.0f}}};
        for (u32 vertex = 0; vertex < 3 && vertex < max_vertices; ++vertex) {
            const std::array<Id, 4> position{ctx.Const(corners[vertex][0]), ctx.Const(corners[vertex][1]),
                                             ctx.Const(0.5f), ctx.Const(1.0f)};
            for (u32 element = 0; element < 4; ++element) {
                ctx.OpStore(ctx.OpAccessChain(ctx.mesh_output_float_pointer, ctx.mesh_vertices,
                                              ctx.Const(vertex), ctx.u32_zero_value, ctx.Const(element)),
                            position[element]);
            }
        }
        ctx.OpStore(ctx.OpAccessChain(ctx.mesh_index_pointer, ctx.mesh_indices, ctx.u32_zero_value),
                    ctx.ConstantComposite(ctx.U32[3], ctx.Const(0U), ctx.Const(1U), ctx.Const(2U)));
        return;
    }
    ctx.OpStore(vertex_count, count);
    ctx.set_mesh_outputs_stand_in = std::make_pair(vertex_count.value, count.value);
    const bool convert_depth{info.convert_depth_mode && !ctx.profile.support_native_ndc};
    for (u32 block = 0; block * 32 < max_vertices; ++block) {
        const Id vertex{ctx.OpIAdd(ctx.U32[1], lane, ctx.Const(block * 32))};
        const auto copy{[&] {
            // Component k of vertex 32 * block + lane is at block * BLOCK + k * 0x80 + lane * 4
            std::array<Id, 4> position{ctx.f32_zero_value, ctx.f32_zero_value, ctx.f32_zero_value,
                                       ctx.Const(1.0f)};
            u32 component{};
            for (size_t output = 0; output < info.mesh_output_masks.size(); ++output) {
                for (u32 element = 0; element < 4; ++element) {
                    if (((info.mesh_output_masks[output] >> element) & 1) == 0) {
                        continue;
                    }
                    const u32 offset{(block * ctx.mesh_vertex_block_bytes + component++ * 0x80) / 4};
                    const Id word{ctx.OpIAdd(ctx.U32[1], lane, ctx.Const(offset))};
                    const Id value{ctx.OpBitcast(
                        ctx.F32[1], ctx.OpLoad(ctx.U32[1], ctx.OpAccessChain(ctx.mesh_workgroup_pointer,
                                                                             ctx.mesh_attributes, word)))};
                    if (output == 0) {
                        position[element] = value;
                    } else {
                        ctx.OpStore(ctx.OpAccessChain(ctx.mesh_output_float_pointer,
                                                      ctx.mesh_generic_outputs[output - 1], vertex,
                                                      ctx.Const(element)),
                                    value);
                    }
                }
            }
            if (convert_depth) {
                position[2] = ctx.OpFMul(ctx.F32[1], ctx.OpFAdd(ctx.F32[1], position[2], position[3]),
                                         ctx.Const(0.5f));
            }
            for (u32 element = 0; element < 4; ++element) {
                ctx.OpStore(ctx.OpAccessChain(ctx.mesh_output_float_pointer, ctx.mesh_vertices, vertex,
                                              ctx.u32_zero_value, ctx.Const(element)),
                            position[element]);
            }
        }};
        if ((block + 1) * 32 > max_vertices) {
            IfThen(ctx, ctx.OpULessThan(ctx.U1, vertex, ctx.Const(max_vertices)), copy);
        } else {
            copy();
        }
    }
    // Component k of primitive 32 * block + lane is at PRIM_BASE + block * 0x80 * components +
    // k * 0x80 + lane * 4, with PRIM_BASE right after the vertex blocks
    const u32 primitive_base{Common::DivCeil(max_vertices, 32U) * ctx.mesh_vertex_block_bytes};
    for (u32 block = 0; ctx.mesh_primitive_components != 0 && block * 32 < max_primitives; ++block) {
        const Id primitive{ctx.OpIAdd(ctx.U32[1], lane, ctx.Const(block * 32))};
        IfThen(ctx, ctx.OpULessThan(ctx.U1, primitive, ctx.Const(max_primitives)), [&] {
            u32 component{};
            for (size_t generic = 0; generic < info.mesh_primitive_masks.size(); ++generic) {
                for (u32 element = 0; element < 4; ++element) {
                    if (((info.mesh_primitive_masks[generic] >> element) & 1) == 0) {
                        continue;
                    }
                    const u32 offset{(primitive_base + block * 0x80 * ctx.mesh_primitive_components +
                                      component++ * 0x80) / 4};
                    const Id word{ctx.OpIAdd(ctx.U32[1], lane, ctx.Const(offset))};
                    const Id value{ctx.OpBitcast(
                        ctx.F32[1], ctx.OpLoad(ctx.U32[1], ctx.OpAccessChain(ctx.mesh_workgroup_pointer,
                                                                             ctx.mesh_attributes, word)))};
                    ctx.OpStore(ctx.OpAccessChain(ctx.mesh_output_float_pointer,
                                                  ctx.mesh_primitive_outputs[generic], primitive,
                                                  ctx.Const(element)),
                                value);
                }
            }
        });
    }
    for (u32 block = 0; block * 32 < max_primitives; ++block) {
        const Id primitive{ctx.OpIAdd(ctx.U32[1], lane, ctx.Const(block * 32))};
        IfThen(ctx, ctx.OpULessThan(ctx.U1, primitive, count), [&] {
            // Triangle p's indices are bytes 4 + 3p, 4 + 3p + 1 and 4 + 3p + 2
            const Id first{ctx.OpIAdd(ctx.U32[1], ctx.OpIMul(ctx.U32[1], primitive, ctx.Const(3U)),
                                      ctx.Const(4U))};
            std::array<Id, 3> indices;
            for (u32 index = 0; index < 3; ++index) {
                const Id byte{ctx.OpIAdd(ctx.U32[1], first, ctx.Const(index))};
                indices[index] = ctx.OpLoad(
                    ctx.U32[1], ctx.OpAccessChain(ctx.mesh_workgroup_pointer, ctx.mesh_index_bytes, byte));
            }
            ctx.OpStore(ctx.OpAccessChain(ctx.mesh_index_pointer, ctx.mesh_indices, primitive),
                        ctx.OpCompositeConstruct(ctx.U32[3], indices[0], indices[1], indices[2]));
        });
    }
}

Id DefineMeshMain(EmitContext& ctx, IR::Program& program) {
    const Id void_function{ctx.TypeFunction(ctx.void_id)};
    const Id body{ctx.OpFunction(ctx.void_id, spv::FunctionControlMask::MaskNone, void_function)};
    for (IR::Block* const block : program.blocks) {
        block->SetDefinition(ctx.OpLabel());
    }
    Traverse(ctx, program);
    ctx.OpFunctionEnd();

    const Id main{ctx.OpFunction(ctx.void_id, spv::FunctionControlMask::MaskNone, void_function)};
    ctx.AddLabel();
    const Id lane{ctx.OpCompositeExtract(ctx.U32[1], ctx.OpLoad(ctx.U32[3], ctx.mesh_local_invocation_id),
                                         0U)};
    // Each workgroup starts with no mesh workgroups to launch and no primitives
    const Id count_pointer{ctx.stage == Stage::Task
                               ? ctx.OpAccessChain(ctx.task_payload_pointer, ctx.task_payload, ctx.Const(1U))
                               : ctx.mesh_primitive_count};
    IfThen(ctx, ctx.OpIEqual(ctx.U1, lane, ctx.u32_zero_value),
           [&] { ctx.OpStore(count_pointer, ctx.u32_zero_value); });
    MeshBarrier(ctx);
    ctx.OpFunctionCall(ctx.void_id, body);
    MeshBarrier(ctx);
    if (ctx.stage == Stage::Task) {
        const Id count{ctx.OpUMin(ctx.U32[1], ctx.OpLoad(ctx.U32[1], count_pointer), ctx.Const(256U))};
        ctx.OpControlBarrier(count, ctx.Const(1U), ctx.Const(1U));
        ctx.emit_mesh_tasks_stand_in = count.value;
    } else {
        EmitMeshOutputs(ctx, lane);
        ctx.OpReturn();
    }
    ctx.OpFunctionEnd();
    return main;
}

// Sirit has no mesh instructions: turn the stand-ins EmitMeshOutputs and DefineMeshMain left into
// OpSetMeshOutputsEXT (same operands as OpStore) and OpEmitMeshTasksEXT (OpControlBarrier's three
// operands plus the payload).
void RewriteMeshStandIns(std::vector<u32>& code, const EmitContext& ctx) {
    if (!ctx.set_mesh_outputs_stand_in && !ctx.emit_mesh_tasks_stand_in) {
        return;
    }
    for (size_t offset = 5; offset < code.size();) {
        const u32 word_count{code[offset] >> 16};
        const auto opcode{static_cast<spv::Op>(code[offset] & 0xFFFFu)};
        if (word_count == 0) {
            return;
        }
        if (opcode == spv::Op::OpStore && word_count == 3 && ctx.set_mesh_outputs_stand_in &&
            code[offset + 1] == ctx.set_mesh_outputs_stand_in->first &&
            code[offset + 2] == ctx.set_mesh_outputs_stand_in->second) {
            code[offset] = (3U << 16) | static_cast<u32>(spv::Op::OpSetMeshOutputsEXT);
        } else if (opcode == spv::Op::OpControlBarrier && word_count == 4 &&
                   ctx.emit_mesh_tasks_stand_in && code[offset + 1] == *ctx.emit_mesh_tasks_stand_in) {
            code[offset] = (5U << 16) | static_cast<u32>(spv::Op::OpEmitMeshTasksEXT);
            code.insert(code.begin() + static_cast<std::ptrdiff_t>(offset) + 4, ctx.task_payload.value);
            offset += 5;
            continue;
        }
        offset += word_count;
    }
}

spv::ExecutionMode ExecutionMode(TessPrimitive primitive) {
    switch (primitive) {
    case TessPrimitive::Isolines:
        return spv::ExecutionMode::Isolines;
    case TessPrimitive::Triangles:
        return spv::ExecutionMode::Triangles;
    case TessPrimitive::Quads:
        return spv::ExecutionMode::Quads;
    }
    throw InvalidArgument("Tessellation primitive {}", primitive);
}

spv::ExecutionMode ExecutionMode(TessSpacing spacing) {
    switch (spacing) {
    case TessSpacing::Equal:
        return spv::ExecutionMode::SpacingEqual;
    case TessSpacing::FractionalOdd:
        return spv::ExecutionMode::SpacingFractionalOdd;
    case TessSpacing::FractionalEven:
        return spv::ExecutionMode::SpacingFractionalEven;
    }
    throw InvalidArgument("Tessellation spacing {}", spacing);
}

void DefineEntryPoint(const IR::Program& program, EmitContext& ctx, Id main) {
    const std::span interfaces(ctx.interfaces.data(), ctx.interfaces.size());
    spv::ExecutionModel execution_model{};
    switch (program.stage) {
    case Stage::Compute: {
        const std::array<u32, 3> workgroup_size{program.workgroup_size};
        execution_model = spv::ExecutionModel::GLCompute;
        ctx.AddExecutionMode(main, spv::ExecutionMode::LocalSize, workgroup_size[0],
                             workgroup_size[1], workgroup_size[2]);
        break;
    }
    case Stage::VertexB:
        execution_model = spv::ExecutionModel::Vertex;
        break;
    case Stage::TessellationControl:
        execution_model = spv::ExecutionModel::TessellationControl;
        ctx.AddCapability(spv::Capability::Tessellation);
        ctx.AddExecutionMode(main, spv::ExecutionMode::OutputVertices, program.invocations);
        break;
    case Stage::TessellationEval:
        execution_model = spv::ExecutionModel::TessellationEvaluation;
        ctx.AddCapability(spv::Capability::Tessellation);
        ctx.AddExecutionMode(main, ExecutionMode(ctx.runtime_info.tess_primitive));
        ctx.AddExecutionMode(main, ExecutionMode(ctx.runtime_info.tess_spacing));
        ctx.AddExecutionMode(main, ctx.runtime_info.tess_clockwise
                                       ? spv::ExecutionMode::VertexOrderCw
                                       : spv::ExecutionMode::VertexOrderCcw);
        break;
    case Stage::Geometry:
        execution_model = spv::ExecutionModel::Geometry;
        ctx.AddCapability(spv::Capability::Geometry);
        if (ctx.profile.support_geometry_streams) {
            ctx.AddCapability(spv::Capability::GeometryStreams);
        }
        switch (ctx.runtime_info.input_topology) {
        case InputTopology::Points:
            ctx.AddExecutionMode(main, spv::ExecutionMode::InputPoints);
            break;
        case InputTopology::Lines:
            ctx.AddExecutionMode(main, spv::ExecutionMode::InputLines);
            break;
        case InputTopology::LinesAdjacency:
            ctx.AddExecutionMode(main, spv::ExecutionMode::InputLinesAdjacency);
            break;
        case InputTopology::Triangles:
            ctx.AddExecutionMode(main, spv::ExecutionMode::Triangles);
            break;
        case InputTopology::TrianglesAdjacency:
            ctx.AddExecutionMode(main, spv::ExecutionMode::InputTrianglesAdjacency);
            break;
        }
        switch (program.output_topology) {
        case OutputTopology::PointList:
            ctx.AddExecutionMode(main, spv::ExecutionMode::OutputPoints);
            break;
        case OutputTopology::LineStrip:
            ctx.AddExecutionMode(main, spv::ExecutionMode::OutputLineStrip);
            break;
        case OutputTopology::TriangleStrip:
            ctx.AddExecutionMode(main, spv::ExecutionMode::OutputTriangleStrip);
            break;
        }
        if (program.info.stores[IR::Attribute::PointSize]) {
            ctx.AddCapability(spv::Capability::GeometryPointSize);
        }
        ctx.AddExecutionMode(main, spv::ExecutionMode::OutputVertices, program.output_vertices);
        ctx.AddExecutionMode(main, spv::ExecutionMode::Invocations, program.invocations);
        if (program.is_geometry_passthrough) {
            if (ctx.profile.support_geometry_shader_passthrough) {
                ctx.AddExtension("SPV_NV_geometry_shader_passthrough");
                ctx.AddCapability(spv::Capability::GeometryShaderPassthroughNV);
            } else {
                LOG_WARNING(Shader_SPIRV, "Geometry shader passthrough used with no support");
            }
        }
        break;
    case Stage::Fragment:
        execution_model = spv::ExecutionModel::Fragment;
        if (ctx.profile.lower_left_origin_mode) {
            ctx.AddExecutionMode(main, spv::ExecutionMode::OriginLowerLeft);
        } else {
            ctx.AddExecutionMode(main, spv::ExecutionMode::OriginUpperLeft);
        }
        if (program.info.stores_frag_depth) {
            ctx.AddExecutionMode(main, spv::ExecutionMode::DepthReplacing);
        }
        if (ctx.runtime_info.force_early_z) {
            ctx.AddExecutionMode(main, spv::ExecutionMode::EarlyFragmentTests);
        }
        if (ctx.profile.support_shader_quad_control && program.info.uses_quad_shuffles) {
            ctx.AddExtension("SPV_KHR_quad_control");
            ctx.AddCapability(spv::Capability::QuadControlKHR);
            ctx.AddExecutionMode(main, spv::ExecutionMode::RequireFullQuadsKHR);
        }
        break;
    case Stage::Task:
        execution_model = spv::ExecutionModel::TaskEXT;
        ctx.AddExecutionMode(main, spv::ExecutionMode::LocalSize, 32U, 1U, 1U);
        break;
    case Stage::Mesh:
        execution_model = spv::ExecutionModel::MeshEXT;
        ctx.AddExecutionMode(main, spv::ExecutionMode::LocalSize, 32U, 1U, 1U);
        ctx.AddExecutionMode(main, spv::ExecutionMode::OutputVertices,
                             ctx.runtime_info.mesh_max_vertices);
        ctx.AddExecutionMode(main, spv::ExecutionMode::OutputPrimitivesEXT,
                             ctx.runtime_info.mesh_max_primitives);
        ctx.AddExecutionMode(main, spv::ExecutionMode::OutputTrianglesEXT);
        break;
    default:
        throw NotImplementedException("Stage {}", program.stage);
    }
    ctx.AddEntryPoint(execution_model, main, "main", interfaces);
}

void SetupDenormControl(const Profile& profile, const IR::Program& program, EmitContext& ctx,
                        Id main_func) {
    const Info& info{program.info};
    if (info.uses_fp32_denorms_flush && info.uses_fp32_denorms_preserve) {
        LOG_DEBUG(Shader_SPIRV, "Fp32 denorm flush and preserve on the same shader");
    } else if (info.uses_fp32_denorms_flush) {
        if (profile.support_fp32_denorm_flush && !profile.has_broken_fp32_denorm_flush) {
            ctx.AddCapability(spv::Capability::DenormFlushToZero);
            ctx.AddExecutionMode(main_func, spv::ExecutionMode::DenormFlushToZero, 32U);
        } else {
            // Drivers will most likely flush denorms by default, no need to warn
        }
    } else if (info.uses_fp32_denorms_preserve) {
        if (profile.support_fp32_denorm_preserve) {
            ctx.AddCapability(spv::Capability::DenormPreserve);
            ctx.AddExecutionMode(main_func, spv::ExecutionMode::DenormPreserve, 32U);
        } else {
            LOG_DEBUG(Shader_SPIRV, "Fp32 denorm preserve used in shader without host support");
        }
    }
    if (!profile.support_separate_denorm_behavior || profile.has_broken_fp16_float_controls) {
        // No separate denorm behavior
        return;
    }
    if (info.uses_fp16_denorms_flush && info.uses_fp16_denorms_preserve) {
        LOG_DEBUG(Shader_SPIRV, "Fp16 denorm flush and preserve on the same shader");
    } else if (info.uses_fp16_denorms_flush) {
        if (profile.support_fp16_denorm_flush) {
            ctx.AddCapability(spv::Capability::DenormFlushToZero);
            ctx.AddExecutionMode(main_func, spv::ExecutionMode::DenormFlushToZero, 16U);
        } else {
            // Same as fp32, no need to warn as most drivers will flush by default
        }
    } else if (info.uses_fp16_denorms_preserve) {
        if (profile.support_fp16_denorm_preserve) {
            ctx.AddCapability(spv::Capability::DenormPreserve);
            ctx.AddExecutionMode(main_func, spv::ExecutionMode::DenormPreserve, 16U);
        } else {
            LOG_DEBUG(Shader_SPIRV, "Fp16 denorm preserve used in shader without host support");
        }
    }
}

void SetupSignedNanCapabilities(const Profile& profile, const IR::Program& program,
                                EmitContext& ctx, Id main_func) {
    if (profile.has_broken_fp16_float_controls && program.info.uses_fp16) {
        return;
    }
    if (program.info.uses_fp16 && profile.support_fp16_signed_zero_nan_preserve) {
        ctx.AddCapability(spv::Capability::SignedZeroInfNanPreserve);
        ctx.AddExecutionMode(main_func, spv::ExecutionMode::SignedZeroInfNanPreserve, 16U);
    }
    if (profile.support_fp32_signed_zero_nan_preserve) {
        ctx.AddCapability(spv::Capability::SignedZeroInfNanPreserve);
        ctx.AddExecutionMode(main_func, spv::ExecutionMode::SignedZeroInfNanPreserve, 32U);
    }
    if (program.info.uses_fp64 && profile.support_fp64_signed_zero_nan_preserve) {
        ctx.AddCapability(spv::Capability::SignedZeroInfNanPreserve);
        ctx.AddExecutionMode(main_func, spv::ExecutionMode::SignedZeroInfNanPreserve, 64U);
    }
}

void SetupTransformFeedbackCapabilities(EmitContext& ctx, Id main_func) {
    if (ctx.runtime_info.xfb_count == 0) {
        return;
    }
    ctx.AddCapability(spv::Capability::TransformFeedback);
    ctx.AddExecutionMode(main_func, spv::ExecutionMode::Xfb);
}

void SetupCapabilities(const Profile& profile, const Info& info, EmitContext& ctx) {
    if (info.uses_sampled_1d) {
        ctx.AddCapability(spv::Capability::Sampled1D);
    }
    if (info.uses_image_1d) {
        ctx.AddCapability(spv::Capability::Image1D);
    }
    if (info.uses_sparse_residency) {
        ctx.AddCapability(spv::Capability::SparseResidency);
    }
    if (info.uses_demote_to_helper_invocation && profile.support_demote_to_helper_invocation) {
        if (profile.supported_spirv < 0x00010600) {
            ctx.AddExtension("SPV_EXT_demote_to_helper_invocation");
        }
        ctx.AddCapability(spv::Capability::DemoteToHelperInvocation);
    }
    if (info.stores[IR::Attribute::ViewportIndex] && profile.support_multi_viewport) {
        ctx.AddCapability(spv::Capability::MultiViewport);
    }
    if (info.stores[IR::Attribute::ViewportMask] && profile.support_viewport_mask) {
        ctx.AddExtension("SPV_NV_viewport_array2");
        ctx.AddCapability(spv::Capability::ShaderViewportMaskNV);
    }
    if (info.stores[IR::Attribute::Layer] || info.stores[IR::Attribute::ViewportIndex]) {
        if (profile.support_viewport_index_layer_non_geometry && ctx.stage != Stage::Geometry) {
            ctx.AddExtension("SPV_EXT_shader_viewport_index_layer");
            ctx.AddCapability(spv::Capability::ShaderViewportIndexLayerEXT);
        }
    }
    if (!profile.support_vertex_instance_id &&
        (info.loads[IR::Attribute::InstanceId] || info.loads[IR::Attribute::VertexId])) {
        ctx.AddExtension("SPV_KHR_shader_draw_parameters");
        ctx.AddCapability(spv::Capability::DrawParameters);
    }
    if ((info.uses_subgroup_vote || info.uses_subgroup_invocation_id ||
         info.uses_subgroup_shuffles || info.uses_subgroup_mask) &&
        profile.support_vote && profile.SupportsSubgroupStage(ctx.stage)) {
        ctx.AddCapability(spv::Capability::GroupNonUniformBallot);
        ctx.AddCapability(spv::Capability::GroupNonUniformShuffle);
        if (info.uses_subgroup_shuffles && profile.support_shuffle_relative) {
            ctx.AddCapability(spv::Capability::GroupNonUniformShuffleRelative);
        }
        if (!profile.warp_size_potentially_larger_than_guest) {
            // vote ops are only used when not taking the long path
            ctx.AddCapability(spv::Capability::GroupNonUniformVote);
        }
    }
    if (info.uses_quad_shuffles) {
        if (profile.support_quad_shuffles) {
            ctx.AddCapability(spv::Capability::GroupNonUniformQuad);
        }
        ctx.AddCapability(spv::Capability::GroupNonUniformShuffle);
    }
    if (info.uses_int64_bit_atomics && profile.support_int64_atomics) {
        ctx.AddCapability(spv::Capability::Int64Atomics);
    }
    if (info.uses_typeless_image_reads && profile.support_typeless_image_loads) {
        ctx.AddCapability(spv::Capability::StorageImageReadWithoutFormat);
    }
    if (info.uses_typeless_image_writes) {
        ctx.AddCapability(spv::Capability::StorageImageWriteWithoutFormat);
    }
    if (info.uses_image_buffers) {
        ctx.AddCapability(spv::Capability::ImageBuffer);
    }
    if (info.uses_sample_id) {
        ctx.AddCapability(spv::Capability::SampleRateShading);
    }
    if (info.uses_derivatives) {
        ctx.AddCapability(spv::Capability::DerivativeControl);
    }
    // TODO: Track this usage
    ctx.AddCapability(spv::Capability::ImageGatherExtended);
    ctx.AddCapability(spv::Capability::ImageQuery);
    ctx.AddCapability(spv::Capability::SampledBuffer);
    if (!ctx.non_uniform_ids.empty()) {
        if (ctx.profile.supported_spirv < 0x00010500)
            ctx.AddExtension("SPV_EXT_descriptor_indexing");
        ctx.AddCapability(spv::Capability::ShaderNonUniform);
        if (ctx.uses_nonuniform_sampled_image) {
            ctx.AddCapability(spv::Capability::SampledImageArrayNonUniformIndexing);
        }
        if (ctx.uses_nonuniform_storage_image) {
            ctx.AddCapability(spv::Capability::StorageImageArrayNonUniformIndexing);
        }
        if (ctx.uses_nonuniform_uniform_texel_buffer) {
            ctx.AddCapability(spv::Capability::UniformTexelBufferArrayNonUniformIndexing);
        }
        if (ctx.uses_nonuniform_storage_texel_buffer) {
            ctx.AddCapability(spv::Capability::StorageTexelBufferArrayNonUniformIndexing);
        }
    }
}

void PatchPhiNodes(IR::Program& program, EmitContext& ctx) {
            // Flatten all leading PHIs from each block into a vector
            std::vector<IR::Inst*> phi_instructions;
            for (IR::Block* block : program.blocks) {
                for (auto it = block->begin(); it != block->end(); ++it) {
                    if (it->GetOpcode() != IR::Opcode::Phi)
                        break;
                    phi_instructions.push_back(&*it);
                }
            }

            if (phi_instructions.empty()) {
                return; // nothing to patch
            }

            // Start "before" first PHI; advance on phi_arg == 0
            size_t phi_index = static_cast<size_t>(-1);

            ctx.PatchDeferredPhi([&](size_t phi_arg, Id parent) -> std::pair<Id, Id> {
                if (phi_arg == 0) {
                    ++phi_index;
                }
                IR::Inst* phi = phi_instructions[phi_index];
                return { ctx.Def(phi->Arg(phi_arg)), parent };
            });
        }

void RewriteOpcodes(std::vector<u32>& code, std::span<const std::pair<u32, spv::Op>> rewrites) {
    if (rewrites.empty()) {
        return;
    }
    size_t offset = 5;
    while (offset + 2 < code.size()) {
        const u32 word_count{code[offset] >> 16};
        const auto opcode{static_cast<spv::Op>(code[offset] & 0xFFFFu)};
        if (word_count == 0) {
            return;
        }
        if (opcode == spv::Op::OpGroupNonUniformShuffleXor ||
            opcode == spv::Op::OpGroupNonUniformQuadBroadcast) {
            const auto it{std::ranges::find(rewrites, code[offset + 2],
                                            &std::pair<u32, spv::Op>::first)};
            if (it != rewrites.end()) {
                code[offset] = (code[offset] & 0xFFFF0000u) | static_cast<u32>(it->second);
            }
        }
        offset += word_count;
    }
}
} // Anonymous namespace

std::vector<u32> EmitSPIRV(const Profile& profile, const RuntimeInfo& runtime_info, IR::Program& program, Bindings& bindings) {
    EmitContext ctx{profile, runtime_info, program, bindings};
    const bool mesh_stage{program.stage == Stage::Task || program.stage == Stage::Mesh};
    const Id main{mesh_stage ? DefineMeshMain(ctx, program) : DefineMain(ctx, program)};
    DefineEntryPoint(program, ctx, main);
    if (profile.support_float_controls) {
        ctx.AddExtension("SPV_KHR_float_controls");
        SetupDenormControl(profile, program, ctx, main);
        SetupSignedNanCapabilities(profile, program, ctx, main);
    }
    SetupCapabilities(profile, program.info, ctx);
    SetupTransformFeedbackCapabilities(ctx, main);
    PatchPhiNodes(program, ctx);
    std::vector<u32> code{ctx.Assemble()};
    RewriteOpcodes(code, ctx.opcode_rewrites);
    RewriteMeshStandIns(code, ctx);
    return code;
}

Id EmitPhi(EmitContext& ctx, IR::Inst* inst) {
    const size_t num_args{inst->NumArgs()};
    boost::container::small_vector<Id, 32> blocks;
    blocks.reserve(num_args);
    for (size_t index = 0; index < num_args; ++index) {
        blocks.push_back(inst->PhiBlock(index)->Definition<Id>());
    }
    // The type of a phi instruction is stored in its flags
    const Id result_type{TypeId(ctx, inst->Flags<IR::Type>())};
    return ctx.DeferredOpPhi(result_type, std::span(blocks.data(), blocks.size()));
}

void EmitVoid(EmitContext&) {}

Id EmitIdentity(EmitContext& ctx, const IR::Value& value) {
    const Id id{ctx.Def(value)};
    if (!Sirit::ValidId(id)) {
        throw NotImplementedException("Forward identity declaration");
    }
    return id;
}

Id EmitConditionRef(EmitContext& ctx, const IR::Value& value) {
    const Id id{ctx.Def(value)};
    if (!Sirit::ValidId(id)) {
        throw NotImplementedException("Forward identity declaration");
    }
    return id;
}

void EmitReference(EmitContext&) {}

void EmitPhiMove(EmitContext&) {
    throw LogicError("Unreachable instruction");
}

void EmitGetZeroFromOp(EmitContext&) {
    throw LogicError("Unreachable instruction");
}

void EmitGetSignFromOp(EmitContext&) {
    throw LogicError("Unreachable instruction");
}

void EmitGetCarryFromOp(EmitContext&) {
    throw LogicError("Unreachable instruction");
}

void EmitGetOverflowFromOp(EmitContext&) {
    throw LogicError("Unreachable instruction");
}

void EmitGetSparseFromOp(EmitContext&) {
    throw LogicError("Unreachable instruction");
}

void EmitGetInBoundsFromOp(EmitContext&) {
    throw LogicError("Unreachable instruction");
}

} // namespace Shader::Backend::SPIRV
