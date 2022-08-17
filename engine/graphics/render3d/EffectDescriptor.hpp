#pragma once

#include "effect.hpp"

namespace shadergen::effect {

namespace OpenGL {
    struct Transform : ASTTransform<Transform> {
        using Base = ASTTransform<Transform>;
        const EffectStageDecl* stageDecl = nullptr;
        constexpr Transform(ASTStorage storage, const EffectStageDecl* stageDecl)
            : Base(storage), stageDecl(stageDecl) { }
        constexpr void transformVarRefExpr(VarRefExpr* e) {
            stageDecl->subpassInputs.decls.forEachEntry([&](const VarDecl* subpassDecl) {
                if (subpassDecl == e->decl)
                    e->decl->declContext = &globalPseudoVarDeclContext;
            });
            Base::transformVarRefExpr(e);
        }
    };
    struct SubpassInputReplacement : SourceReplacement {
        int_t subpassIndex;
        constexpr SubpassInputReplacement()
            : subpassIndex(-1) { }
        constexpr SubpassInputReplacement(int_t subpassIndex)
            : SourceReplacement(3), subpassIndex(subpassIndex) { }
    };
    struct StmtGenerator : GLSLStmtGenerator<StmtGenerator> {
        using Base = GLSLStmtGenerator<StmtGenerator>;

        const EffectStageDecl* stageDecl;
        PseudoVector<SubpassInputReplacement> subpassReplacements;

        constexpr StmtGenerator(PseudoVector<char> sourceBuffer, const EffectStageDecl* stageDecl, PseudoVector<SubpassInputReplacement> subpassReplacements)
            : Base(sourceBuffer), stageDecl(stageDecl), subpassReplacements(subpassReplacements) { }

        constexpr void emitVarName(const VarDecl* decl) {
            int_t counter = 0;
            bool found = false;
            stageDecl->subpassInputs.decls.forEachEntry([&](const VarDecl* oldDecl) {
                if (found)
                    return;
                if (oldDecl == decl)
                    found = true;
                else
                    counter++;
            });
            if (found) {
                SubpassInputReplacement rep { counter };
                out() << "subpassInput_" << rep;
                subpassReplacements.pushBack(rep);
                return;
            }
            Base::emitVarName(decl);
        }
    };
}

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
    PseudoVector<CrossEffectVarUsage> out;
    constexpr CrossEffectVarUsageFinder(PseudoVector<CrossEffectVarUsage> out)
        : out(out) { }
    constexpr bool checkForUpdate(const VarRefExpr* l, const Expr* e) {
        if (e->stmtKind != StmtKind::VarRefExpr)
            return false;
        const auto* r = (const VarRefExpr*)e;
        if (l->decl != r->decl)
            return false;
        return true;
    }
    constexpr CrossEffectVarUsage* findPrev(const VarDecl* decl) {
        for (auto& usage : out) {
            if (usage.varDecl == decl)
                return &usage;
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
            out.pushBack({ e->decl, usage, op });
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

struct ConstexprEffectDescriptor {
    struct VariableDescriptor {
        const Type* type = nullptr;
        const char* nameBegin = nullptr;
        int_t nameLength = 0;
        const int_t* offsetInStruct = nullptr;
        constexpr VariableDescriptor() = default;
        constexpr VariableDescriptor(const Type* type, std::string_view name, const int_t* offset)
            : type(type), nameBegin(name.begin()), nameLength(name.length()), offsetInStruct(offset) { }
        constexpr std::string_view name() const { return { nameBegin, (size_t)nameLength }; }
    };
    static constexpr void extractVariables(PseudoVector<VariableDescriptor> out, const EffectStageDriver& stage, const VarDeclContext& declContext) {
        declContext.decls.forEachEntry([&](const VarDecl* decl) {
            out.pushBack({ decl->type, decl->name, stage.findOffsetFromVarDecl(decl->prevDecl) });
        });
    }
    struct StageDescriptor {
        OversizedArray<VariableDescriptor, 100> uniforms;
        OversizedArray<CrossEffectVarUsage, 100> ceUsages;
        OversizedArray<VariableDescriptor, 100> subpassInputs;
        OversizedArray<OpenGL::SubpassInputReplacement, 100> subpassReplacements;
        OversizedArray<char, 10000> source;

        constexpr void set(const EffectStageDriver& stage) {
            extractVariables(uniforms, stage, stage.stageDecl()->uniforms);
            extractVariables(subpassInputs, stage, stage.stageDecl()->subpassInputs);

            CrossEffectVarUsageFinder usageFinder { ceUsages };
            usageFinder.dispatchVisit(stage.stageDecl()->entryPoint);

            BasicASTStorage<1000, 10000> storage;
            OpenGL::Transform transform1 { storage, stage.stageDecl() };
            GLSLStmtTransform transform2 { storage };
            OpenGL::StmtGenerator generator { source, stage.stageDecl(), subpassReplacements };
            StageDecl* transformed = transform2.createStageDecl(*transform1.createStageDecl(*stage.stageDecl()));
            generator.dispatchVisit(transformed->entryPoint);
        }
    };
    OversizedArray<VariableDescriptor, 100> attributes;
    OversizedArray<const UserType*, 100> userTypes;
    StageDescriptor vert;
    StageDescriptor frag;

    using DefineFunc_t = void (*)(EffectDriver&);
    constexpr ConstexprEffectDescriptor(DefineFunc_t defineFunc) {
        EffectDriver effect;
        defineFunc(effect);
        extractVariables(attributes, effect.vert, effect.vert.stageDecl()->inputs);
        vert.set(effect.vert);
        frag.set(effect.frag);
        UserTypeRecorder typeRecorder { userTypes };
        typeRecorder.recordStage(effect.vert.stageDecl());
        typeRecorder.recordStage(effect.frag.stageDecl());
    }
};

struct EffectDescriptor {
    struct VariableDescriptor {
        const Type* type;
        std::string_view name;
        int_t offsetInStruct;
    };
    struct VariableArray {
        ArrayView<const VariableDescriptor> view;
        constexpr operator ArrayView<const VariableDescriptor>() const { return view; }

        template<ConstexprEffectDescriptor::VariableDescriptor... descriptors>
        struct StaticArrayHolder {
            static inline const std::array<VariableDescriptor, sizeof...(descriptors)> array { { descriptors.type, descriptors.name(), *descriptors.offsetInStruct }... };
        };
        template<auto oversized, int_t... I>
        static constexpr VariableArray make_helper(stx::int_sequence<I...>) {
            return { StaticArrayHolder<oversized.data[I]...>::array };
        }
        template<auto oversized>
        static constexpr VariableArray make() {
            return make_helper<oversized>(stx::make_int_sequence<oversized.size()>());
        }
    };
    struct StageDescriptor {
        ArrayView<const VariableDescriptor> uniforms;
        ArrayView<const CrossEffectVarUsage> ceUsages;
        ArrayView<const VariableDescriptor> subpassInputs;
        ArrayView<const OpenGL::SubpassInputReplacement> subpassReplacements;
        std::string_view source;

        template<ConstexprEffectDescriptor::StageDescriptor stage>
        constexpr void set() {
            uniforms = VariableArray::make<stage.uniforms>();
            ceUsages = staticView<stage.ceUsages>();
            subpassInputs = VariableArray::make<stage.subpassInputs>();
            subpassReplacements = staticView<stage.subpassReplacements>();
            ArrayView<const char> sourceView = staticView<stage.source>();
            source = { sourceView.begin(), sourceView.end() };
        }
    };
    ArrayView<const VariableDescriptor> attributes;
    ArrayView<const UserType* const> userTypes;
    StageDescriptor vert;
    StageDescriptor frag;

    template<typename E>
    constexpr EffectDescriptor(std::type_identity<E>) {
        constexpr ConstexprEffectDescriptor effect { E::defineEffect };
        attributes = VariableArray::make<effect.attributes>();
        userTypes = staticView<effect.userTypes>();
        vert.set<effect.vert>();
        frag.set<effect.frag>();
    }
};

/*#define DEFINE_EFFECT(effect_type) \
    template<>                     \
    constinit const ::shadergen::effect::EffectDescriptor(::shadergen::effect::descriptorStore<effect_type>) { ::std::type_identity<effect_type>() };
#define DEFINE_BUILTIN_EFFECT(effect_type) \
    template<>                             \
    constinit const ::shadergen::effect::EffectDescriptor(descriptorStore<effect_type>) { ::std::type_identity<effect_type>() };*/
#define DEFINE_EFFECT(e)
#define DEFINE_BUILTIN_EFFECT(e)

struct BuiltinPositionEffect {
    using Self = BuiltinPositionEffect;
    struct MatrixUniforms {
        BEGIN_GEN_STRUCT(MatrixUniforms);
        Mat4x4 GEN_FIELD(viewProjectionMatrix);
        Mat4x4 GEN_FIELD(worldViewMatrix);
        END_GEN_STRUCT();
    };
    GraphicsPtr<MatrixUniforms> matrixUniforms;
    static constexpr void defineEffect(shadergen::effect::EffectDriver& prog) {
        using namespace shadergen::effect::code;
        auto& vert = prog.vert;
        auto& frag = prog.frag;
        auto matrices = vert.uniform<&Self::matrixUniforms>("u_matrices");

        auto [screenPosVert, screenPosFrag] = prog.varying<Vec4>("v_screenPos");
        vert, screenPosVert = matrices.viewProjectionMatrix * modelPosition;
        vert, screenPosition = screenPosVert;
        frag, screenPosition = screenPosFrag;

        auto [worldPosVert, worldPosFrag] = prog.varying<Vec3>("v_worldPos");
        vert, worldPosVert = (matrices.worldViewMatrix * screenPosVert).xyz();
        vert, worldPosition = worldPosVert;
        frag, worldPosition = worldPosFrag;
    }
};
DEFINE_BUILTIN_EFFECT(BuiltinPositionEffect);

inline void generateSource(ArrayView<const EffectDescriptor*> descriptors) {
    struct DagNode {
        const EffectDescriptor* descriptor = nullptr;
        std::string debugName = {};
    };
    using DAG = DirectedAcyclicGraph<DagNode>;
    using NodeHandle = DAG::NodeHandle;
    struct VarInfo {
        NodeHandle afterWrite, beforeRead;
    };

    using UsageType = CrossEffectVarUsage::UsageType;
    auto handleStage = [&](EffectDescriptor::StageDescriptor EffectDescriptor::*stage) {
        DAG dag;
        std::unordered_map<const VarDecl*, VarInfo> varMap;
        int_t counter = 1;
        for (const auto* effectDesc : descriptors) {
            NodeHandle effectNode = dag.newNode({ effectDesc, std::string("effect ") + std::to_string(counter++) });
            const auto& stageDesc = effectDesc->*stage;
            for (auto usage : stageDesc.ceUsages) {
                auto it = varMap.find(usage.varDecl);
                if (it == varMap.end()) {
                    VarInfo varInfo { dag.newNode({ nullptr, std::string(usage.varDecl->name) + " aW" }), dag.newNode({ nullptr, std::string(usage.varDecl->name) + " bR" }) };
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
        for (const auto& pair : varMap) {
            VERIFY(ShaderGen, dag.parents(pair.second.afterWrite).size() == 1);
        }
        struct Generator : GLSLGeneratorBase<Generator> {
            using Base = GLSLGeneratorBase<Generator>;
            Generator(PseudoVector<char> sourceBuffer)
                : Base(sourceBuffer) { }
        };
        OversizedArray<char, 10000> buffer;
        Generator generator { buffer };
        for (const auto* effectDesc : descriptors) {
            const auto& stageDesc = effectDesc->*stage;
            for (auto uniform : stageDesc.uniforms)
                generator.out() << "uniform " << uniform.type << " " << uniform.name << ";\n";
        }
        bool b = dag.topologicalOrdering([&](NodeHandle nodeHandle) {
            auto& node = dag[nodeHandle];
        });
        VDUMP(b);
    };
    handleStage(&EffectDescriptor::vert);
    handleStage(&EffectDescriptor::frag);
}
}
