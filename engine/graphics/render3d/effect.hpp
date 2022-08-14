#pragma once

#include "containers/DirectedAcyclicGraph.hpp"
#include "graphics/shadergen/BuildDriver.hpp"
#include "graphics/shadergen/GLSLGenerator.hpp"
#include <unordered_map>

template<typename T>
struct GPtr { };

template<typename T, int_t N>
struct OversizedArray {
    using value_type = T;
    T data[N] = {};
    int_t size = 0;

    constexpr void pushBack(T item) {
        data[size++] = item;
    }
};

namespace shadergen::effect {

template<typename T, typename E>
inline int_t offsetOfMemberPtr(T E::*ptr) {
    const E* e = nullptr;
    const T* t = &(e->*ptr);
    return (const std::byte*)t - (const std::byte*)e;
}

template<typename T>
struct CXXTypeFromMemberPtr;
template<typename T, typename E>
struct CXXTypeFromMemberPtr<GPtr<T> E::*> {
    using type = std::remove_extent_t<T>;
};

namespace {
    template<auto memberPtr>
    struct StaticOffsetHolder {
        static inline const int_t offset = offsetOfMemberPtr(memberPtr);
    };
};

// Driver
struct EffectStageDriver : StageDriver {
    struct EffectVariableOffset {
        const VarDecl* decl = nullptr;
        const int_t* offsetPtr = nullptr;
    };
    OversizedArray<EffectVariableOffset, 300> offsets;

    constexpr EffectStageDriver(Storage* storage)
        : StageDriver(storage) { }

    constexpr const int_t* findOffsetInStruct(const VarDecl* decl) const {
        for (int_t i = 0; i < offsets.size; i++) {
            if (offsets.data[i].decl == decl)
                return offsets.data[i].offsetPtr;
        }
        VERIFY_NOT_REACHED(ShaderGen);
        return nullptr;
    }
    template<auto memberPtr>
    constexpr DriverVariable<typename CXXTypeFromMemberPtr<decltype(memberPtr)>::type> declareEffectVariable(std::string_view name, VarDeclContext* declContext) {
        return DriverVariable<typename CXXTypeFromMemberPtr<decltype(memberPtr)>::type>(
            name,
            [&](const VarDecl* driverDecl) constexpr {
                createVarDecl(driverDecl, declContext);
                auto& off = offsets.data[offsets.size++];
                off.decl = driverDecl;
                off.offsetPtr = &StaticOffsetHolder<memberPtr>::offset;
            });
    }
    template<auto memberPtr>
    constexpr auto uniform(std::string_view name) {
        return declareEffectVariable<memberPtr>(name, &stageDecl->uniforms);
    }
    template<auto memberPtr>
    constexpr auto input(std::string_view name) {
        return declareEffectVariable<memberPtr>(name, &stageDecl->inputs);
    }
    template<auto memberPtr>
    constexpr auto output(std::string_view name) {
        return declareEffectVariable<memberPtr>(name, &stageDecl->outputs);
    }
};

struct EffectDriver {
    using Storage = BlockDriver::Storage;
    Storage storage;
    EffectStageDriver vert { &storage };
    EffectStageDriver frag { &storage };
};

// cross effect variables
enum class CumulativeUpdateOp {
    NONE,
    ADD,
    MULTIPLY,
    AND,
    OR,
    MIN,
    MAX,
};
template<typename T>
struct CrossEffectVariable : DriverVariable<T> {
    using Base = DriverVariable<T>;
    CumulativeUpdateOp cumOp;
    constexpr CrossEffectVariable(std::string_view name, CumulativeUpdateOp cumOp = CumulativeUpdateOp::NONE)
        : DriverVariable<T>(name, [](const VarDecl*) constexpr {}, &globalPseudoVarDeclContext), cumOp(cumOp) { }
    constexpr void update(const get_wrapper<T>& value) {
        using namespace shadergen::code;
        switch (cumOp) {
        case CumulativeUpdateOp::ADD:
            return *this = *this + value;
        case CumulativeUpdateOp::MULTIPLY:
            return *this = *this * value;
        case CumulativeUpdateOp::AND:
            return *this = *this && value;
        case CumulativeUpdateOp::OR:
            return *this = *this || value;
        case CumulativeUpdateOp::MIN:
            return *this = min(*this, value);
        case CumulativeUpdateOp::MAX:
            return *this = max(*this, value);
        default:
            VERIFY_NOT_REACHED(ShaderGen);
            return *this = value;
        }
    }
    using Base::operator=;
    constexpr AssignmentStmt operator=(const CrossEffectVariable<T>& right) const {
        return { this, right.wrappedExpr };
    }
};
struct CrossEffectVarUsage {
    enum class UsageType {
        Write = 0,
        Update = 1,
        Read = 2,
    };
    const VarDecl* varDecl;
    UsageType usage;
    CumulativeUpdateOp op;
};
struct CrossEffectVarUsageFinder : ASTVisitor<CrossEffectVarUsageFinder> {
    using Base = ASTVisitor<CrossEffectVarUsageFinder>;
    using UsageType = CrossEffectVarUsage::UsageType;
    CrossEffectVarUsage* begin;
    CrossEffectVarUsage* end;
    constexpr CrossEffectVarUsageFinder(CrossEffectVarUsage* begin)
        : begin(begin), end(begin) { }
    constexpr bool checkForUpdate(const VarRefExpr* l, const Expr* e) {
        if (e->stmtKind != StmtKind::VarRefExpr)
            return false;
        const auto* r = (const VarRefExpr*)e;
        if (l->decl != r->decl)
            return false;
        return true;
    }
    constexpr CrossEffectVarUsage* findPrev(const VarDecl* decl) {
        for (auto* it = begin; it < end; it++) {
            if (it->varDecl == decl)
                return it;
        }
        return nullptr;
    }
    constexpr void emitUsage(const VarRefExpr* e, UsageType usage, CumulativeUpdateOp op = CumulativeUpdateOp::NONE) {
        /*if (usage == UsageType::Update && op == CumulativeUpdateOp::NONE) {
            VERIFY_NOT_REACHED(ShaderGen);
            return;
        }*/
        auto* prev = findPrev(e->decl);
        if (!prev) {
            *(end++) = { e->decl, usage, op };
            return;
        }
        if (prev->usage == UsageType::Write && usage == UsageType::Read)
            return;
        if (prev->usage == UsageType::Update && usage == UsageType::Update && prev->op == op)
            return;
        VERIFY_NOT_REACHED(ShaderGen);
    }
    constexpr void visitAssignmentStmt(const AssignmentStmt* s) {
        if (s->left->stmtKind != StmtKind::VarRefExpr)
            return Base::visitAssignmentStmt(s);
        const auto* l = (const VarRefExpr*)s->left;
        if (l->decl->declContext != &globalPseudoVarDeclContext)
            return Base::visitAssignmentStmt(s);
        if (s->right->stmtKind == StmtKind::ElementwiseOperatorExpr) {
            const auto* e = (const ElementwiseOperatorExpr*)s->right;
            if (checkForUpdate(l, e->left)) {
                dispatchVisit(e->right);
                emitUsage(l, UsageType::Update);
                return;
            }
        } else if (s->right->stmtKind == StmtKind::CallExpr) {
            const auto* e = (const CallExpr*)s->right;
            if (!e->arguments.empty() && checkForUpdate(l, e->arguments.front())) {
                bool first = true;
                e->arguments.forEachEntry([&](const Expr* arg) {
                    if (first)
                        first = false;
                    else
                        dispatchVisit(arg);
                });
                emitUsage(l, UsageType::Update);
                return;
            }
        }
        dispatchVisit(s->right);
        emitUsage(l, UsageType::Write);
    };
    constexpr void visitUpdateStmt(const UpdateStmt* s) {
        if (s->left->stmtKind != StmtKind::VarRefExpr)
            return Base::visitUpdateStmt(s);
        const auto* l = (const VarRefExpr*)s->left;
        if (l->decl->declContext != &globalPseudoVarDeclContext)
            return Base::visitUpdateStmt(s);
        dispatchVisit(s->right);
        emitUsage(l, UsageType::Update);
    }
    constexpr void visitVarRefExpr(const VarRefExpr* e) {
        if (e->decl->declContext == &globalPseudoVarDeclContext)
            emitUsage(e, UsageType::Read);
    }
};

// constexpr descriptor
struct ConstVariableDescriptor {
    const Type* type;
    const int_t* offsetInStruct;
};
struct ConstStageDescriptor {
    OversizedArray<ConstVariableDescriptor, 100> uniforms;
    OversizedArray<CrossEffectVarUsage, 100> ceUsages;
    OversizedArray<char, 10000> source;
};
struct ConstEffectDescriptor {
    OversizedArray<ConstVariableDescriptor, 100> attributes;
    OversizedArray<const UserType*, 100> userTypes;
    ConstStageDescriptor vert;
    ConstStageDescriptor frag;
};
constexpr int_t fillConstVariableArray(ConstVariableDescriptor* end, const EffectStageDriver& stage, const VarDeclContext& declContext) {
    ConstVariableDescriptor* begin = end;
    declContext.decls.forEachEntry([&](const VarDecl* decl) {
        *(end++) = { decl->type, stage.findOffsetInStruct(decl->prevDecl) };
    });
    return end - begin;
}
constexpr void fillConstStageDescriptor(ConstStageDescriptor& out, const EffectStageDriver& in) {
    out.uniforms.size = fillConstVariableArray(out.uniforms.data, in, in.stageDecl->uniforms);

    CrossEffectVarUsageFinder usageFinder { out.ceUsages.data };
    usageFinder.dispatchVisit(in.stageDecl->entryPoint);
    out.ceUsages.size = usageFinder.end - usageFinder.begin;
    BlockDriver::Storage storage;
    GLSLStmtTransform transform { &storage };
    StageDecl* transformed = transform.createStageDecl(*in.stageDecl);
    GLSLStmtGenerator generator { out.source.data };
    generator.dispatchVisit(transformed->entryPoint);
    out.source.size = generator.end - out.source.data;
}
using DefineFunc_t = void (*)(EffectDriver&);
constexpr ConstEffectDescriptor createConstEffectDescriptor(DefineFunc_t defineFunc) {
    EffectDriver effect;
    defineFunc(effect);
    ConstEffectDescriptor out;
    out.attributes.size = fillConstVariableArray(out.attributes.data, effect.vert, effect.vert.stageDecl->inputs);
    fillConstStageDescriptor(out.vert, effect.vert);
    fillConstStageDescriptor(out.frag, effect.frag);
    UserTypeRecorder typeRecorder { out.userTypes.data };
    typeRecorder.recordStage(effect.vert.stageDecl);
    typeRecorder.recordStage(effect.frag.stageDecl);
    out.userTypes.size = typeRecorder.end - out.userTypes.data;
    return out;
}

// descriptor
struct VariableDescriptor {
    const Type* type;
    int_t offsetInStruct;
};
struct StageDescriptor {
    ArrayView<const VariableDescriptor> uniforms;
    ArrayView<const CrossEffectVarUsage> ceUsages;
    std::string_view source;
};
struct EffectDescriptor {
    ArrayView<const VariableDescriptor> attributes;
    ArrayView<const UserType* const> userTypes;
    StageDescriptor vert;
    StageDescriptor frag;
};
template<ConstVariableDescriptor... descriptors>
struct StaticVariableDescriptorArray {
    static inline const VariableDescriptor array[] = { { descriptors.type, *descriptors.offsetInStruct }... };
};
template<auto oversized, int_t... I>
constexpr ArrayView<const VariableDescriptor> createVariableDescriptorArray_helper(stx::int_sequence<I...>) {
    return StaticVariableDescriptorArray<oversized.data[I]...>::array;
}
template<auto oversized>
constexpr ArrayView<const VariableDescriptor> createVariableDescriptorArray() {
    if constexpr (oversized.size == 0) // this is necessary because 0 sized arrays aren't allowed
        return {};
    else
        return createVariableDescriptorArray_helper<oversized>(stx::make_int_sequence<oversized.size>());
}
template<auto oversized>
constexpr auto copyFromOversized() {
    std::array<typename decltype(oversized)::value_type, oversized.size> array;
    std::copy_n(oversized.data, oversized.size, array.data());
    return array;
}
template<auto in>
constexpr const auto& makeStatic() {
    return in;
}
template<auto oversized>
constexpr const auto& oversizedToArray() {
    return makeStatic<copyFromOversized<oversized>()>();
}
template<auto oversized>
constexpr std::string_view createSource() {
    const auto& array = oversizedToArray<oversized>();
    return { array.data(), array.size() };
}
template<ConstStageDescriptor stage>
constexpr StageDescriptor createStageDescriptor() {
    return { createVariableDescriptorArray<stage.uniforms>(), oversizedToArray<stage.ceUsages>(), createSource<stage.source>() };
}
template<typename E>
constexpr EffectDescriptor createEffectDescriptor() {
    constexpr ConstEffectDescriptor effect = createConstEffectDescriptor(E::defineEffect);
    return { createVariableDescriptorArray<effect.attributes>(), oversizedToArray<effect.userTypes>(), createStageDescriptor<effect.vert>(), createStageDescriptor<effect.frag>() };
}
template<typename E>
constexpr EffectDescriptor descriptorStore = {};

struct EffectHandle {
    const EffectDescriptor* descriptor = nullptr;
    const void* effect = nullptr;
    constexpr EffectHandle() { }
    template<typename E>
    constexpr EffectHandle(const E* effect)
        : descriptor(&descriptorStore<E>), effect(effect) { }
    constexpr EffectHandle(const EffectDescriptor* desc, const void* effect)
        : descriptor(desc), effect(effect) { }
};

inline void generateSource(ArrayView<const EffectHandle> effectHandles) {
    struct DagNode {
        std::string name = {};
    };
    using DAG = DirectedAcyclicGraph<DagNode>;
    using NodeHandle = DAG::NodeHandle;
    struct VarInfo {
        NodeHandle afterWrite, beforeRead;
    };

    using UsageType = CrossEffectVarUsage::UsageType;
    auto handleStage = [&](StageDescriptor EffectDescriptor::*stage) {
        DAG dag;
        std::unordered_map<const VarDecl*, VarInfo> varMap;
        int_t counter = 1;
        for (auto effectHandle : effectHandles) {
            NodeHandle effectNode = dag.newNode({ std::string("effect ") + std::to_string(counter++) });
            const auto& stageDesc = effectHandle.descriptor->*stage;
            for (auto usage : stageDesc.ceUsages) {
                auto it = varMap.find(usage.varDecl);
                if (it == varMap.end()) {
                    VarInfo varInfo { dag.newNode({ std::string(usage.varDecl->name) + " aW" }), dag.newNode({ std::string(usage.varDecl->name) + " bR" }) };
                    dag.addChild(varInfo.afterWrite, varInfo.beforeRead);
                    it = varMap.insert({ usage.varDecl, varInfo }).first;
                }
                VarInfo& varInfo = it->second;
                switch (usage.usage) {
                case UsageType::Write:
                    dag.addChild(effectNode, varInfo.afterWrite);
                    break;
                case UsageType::Update:
                    dag.addChild(varInfo.afterWrite, effectNode);
                    dag.addChild(effectNode, varInfo.beforeRead);
                    break;
                case UsageType::Read:
                    dag.addChild(varInfo.beforeRead, effectNode);
                    break;
                default:
                    VERIFY_NOT_REACHED(ShaderGen);
                    break;
                }
            }
        }
        log("all notes:");
        dag.forEachNode([&](NodeHandle node) {
            log(" {}", dag[node].name);
        });
        log("ordered:");
        bool b = dag.topologicalOrdering([&](NodeHandle node) {
            log(" {}", dag[node].name);
        });
        VDUMP(b);
    };
    handleStage(&EffectDescriptor::vert);
    handleStage(&EffectDescriptor::frag);
}
}

DEFINE_SECTION(const shadergen::effect::EffectDescriptor, effects)
#define DEFINE_EFFECT(effect_type) \
    template<>                     \
    constinit IN_SECTION(effects) const shadergen::effect::EffectDescriptor shadergen::effect::descriptorStore<effect_type> = shadergen::effect::createEffectDescriptor<effect_type>();
