#pragma once

#include "effect.hpp"
#include "graphics/shadergen/CompressedAST.hpp"
#include "graphics/shadergen/ASTDumper.hpp"

namespace shadergen::effect {

namespace OpenGL {
    struct Transform : ASTTransform<Transform> {
        using Base = ASTTransform<Transform>;
        constexpr Transform(ASTStorage storage)
            : Base(storage) { }
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

        PseudoVector<SubpassInputReplacement> subpassReplacements;

        constexpr StmtGenerator(PseudoVector<char> sourceBuffer, PseudoVector<SubpassInputReplacement> subpassReplacements)
            : Base(sourceBuffer), subpassReplacements(subpassReplacements) { }

        constexpr void emitVarName(const VarDecl* decl) {
            if (decl->modifiers.entryCount == 2 && decl->modifiers.front() == &EffectStageDriver::subpassIndicator) {
                const auto* indexMod = static_cast<const EffectStageDriver::SubpassIndexModifier*>(decl->modifiers.back());
                SubpassInputReplacement rep { indexMod->subpassIndex };
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
    struct VariableDescriptorBase {
        const Type* type = nullptr;
        const char* nameBegin = nullptr;
        int_t nameLength = 0;
        bool uniform = false;
        bool input = false;
        bool output = false;
        constexpr VariableDescriptorBase() = default;
        constexpr VariableDescriptorBase(const Type* type, std::string_view name)
            : type(type), nameBegin(name.data()), nameLength(name.length()) { }
        constexpr std::string_view name() const { return { nameBegin, (size_t)nameLength }; }
    };
    struct VariableDescriptor : VariableDescriptorBase {
        const int_t* offsetInStruct = nullptr;
    };
    static constexpr void extractVariables(PseudoVector<VariableDescriptor> out, const EffectStageDriver& stage, const VarDeclContext& declContext) {
        declContext.decls.forEachEntry([&](const VarDecl* decl) {
            auto& desc = out.pushBack({ { decl->type, decl->name }, stage.findOffsetFromVarDecl(decl->prevDecl) });
            decl->modifiers.forEachEntry([&](const VarDeclModifier* mod) {
                if (mod == &uniformIndicator)
                    desc.uniform = true;
                if (mod == &inputIndicator)
                    desc.input = true;
                if (mod == &outputIndicator)
                    desc.output = true;
            });
        });
    }
    struct StageDescriptor {
        OversizedArray<VariableDescriptor, 100> globals;
        OversizedArray<CrossEffectVarUsage, 100> ceUsages;
        OversizedArray<OpenGL::SubpassInputReplacement, 100> subpassReplacements;
        OversizedArray<char, 10000> source;

        constexpr void set(const EffectStageDriver& stage) {
            extractVariables(globals, stage, stage.stageDecl()->globals);

            CrossEffectVarUsageFinder usageFinder { ceUsages };
            usageFinder.dispatchVisit(stage.stageDecl()->entryPoint);

            BasicASTStorage<1000, 10000> storage;
            OpenGL::Transform transform1 { storage };
            GLSLStmtTransform transform2 { storage };
            OpenGL::StmtGenerator generator { source, subpassReplacements };
            StageDecl* transformed = transform2.createStageDecl(*transform1.createStageDecl(*stage.stageDecl()));
            generator.dispatchVisit(transformed->entryPoint);
        }
    };
    OversizedArray<const UserType*, 100> userTypes;
    StageDescriptor vert;
    StageDescriptor frag;

    constexpr void set(const EffectDriver& effect) {
        vert.set(effect.vert);
        frag.set(effect.frag);
        UserTypeRecorder typeRecorder { userTypes };
        typeRecorder.visitStageDecl(effect.vert.stageDecl());
        typeRecorder.visitStageDecl(effect.frag.stageDecl());
    }
};

struct EffectDescriptor {
    using VariableDescriptorBase = ConstexprEffectDescriptor::VariableDescriptorBase;
    struct VariableDescriptor : VariableDescriptorBase {
        int_t offsetInStruct = -1;
    };
    struct VariableArray {
        ArrayView<const VariableDescriptor> view = {};
        constexpr operator ArrayView<const VariableDescriptor>() const { return view; }

        template<ConstexprEffectDescriptor::VariableDescriptor... descriptors>
        struct StaticArrayHolder {
            static inline const VariableDescriptor array[sizeof...(descriptors)] = { { static_cast<const VariableDescriptorBase&>(descriptors), descriptors.offsetInStruct ? *descriptors.offsetInStruct : -1 }... };
        };
        template<typename T>
        struct ArraySize;
        template<typename T, int_t N>
        struct ArraySize<T[N]> {
            static constexpr int_t size = N;
        };
        template<auto oversized, int_t... I>
        static constexpr VariableArray make_helper(stx::int_sequence<I...>) {
            if constexpr (oversized.size() == 0) {
                return {};
            } else {
                using ArrayHolder = StaticArrayHolder<oversized.data[I]...>;
                static_assert(ArraySize<decltype(ArrayHolder::array)>::size == oversized.size());
                return { ArrayHolder::array };
            }
        }
        template<auto oversized>
        static constexpr VariableArray make() {
            return make_helper<oversized>(stx::make_int_sequence<oversized.size()>());
        }
    };
    struct StageDescriptor {
        ArrayView<const VariableDescriptor> globals;
        ArrayView<const CrossEffectVarUsage> ceUsages;
        ArrayView<const OpenGL::SubpassInputReplacement> subpassReplacements;
        std::string_view source;

        template<ConstexprEffectDescriptor::StageDescriptor stage>
        constexpr void set() {
            globals = VariableArray::make<stage.globals>();
            ceUsages = staticView<stage.ceUsages>();
            subpassReplacements = staticView<stage.subpassReplacements>();
            ArrayView<const char> sourceView = staticView<stage.source>();
            source = { sourceView.begin(), (size_t)sourceView.size() };
        }
    };
    std::string_view name;
    ArrayView<const UserType* const> userTypes;
    StageDescriptor vert;
    StageDescriptor frag;
    const StageDecl* vertAST = nullptr;
    const StageDecl* fragAST = nullptr;

    template<typename E>
    struct ConstexprResult {
        BasicASTStorage<1000, 10000> storage;
        const StageDecl* vert = nullptr;
        const StageDecl* frag = nullptr;
        ConstexprEffectDescriptor descriptor;

        constexpr ConstexprResult(std::type_identity<E>) {
            EffectDriver driver { storage };
            vert = driver.vert.stageDecl();
            frag = driver.frag.stageDecl();
            E::defineEffect(driver);
            descriptor.set(driver);
            PrevDeclStripper stripper1;
            stripper1.visitStageDecl(vert);
            stripper1.visitStageDecl(frag);
            TemporaryModifierStripper stripper2;
            stripper2.visitStageDecl(vert);
            stripper2.visitStageDecl(frag);
        }
    };
    template<typename E>
    static constexpr ConstexprResult<E> constexprResults = { std::type_identity<E>() };

    template<typename E>
    constexpr EffectDescriptor(std::type_identity<E>, std::string_view effectName)
        : name(effectName) {
        constexpr const ConstexprResult<E>& result = constexprResults<E>;
        constexpr const ConstexprEffectDescriptor& effect = result.descriptor;
        userTypes = staticView<effect.userTypes>();
        vert.set<effect.vert>();
        frag.set<effect.frag>();
        vertAST = compressStage<result.vert>();
        fragAST = compressStage<result.frag>();
    }

    void dumpAST() const {
        ASTDumper dumper;
        log("----------- {} vertex -----------", name);
        dumper.dump(vertAST);
        log("---------- {} fragment ----------", name);
        dumper.dump(fragAST);
    }
};

#define DEFINE_EFFECT(effect_type) \
    template<>                     \
    constinit const ::shadergen::effect::EffectDescriptor(::shadergen::effect::descriptorStore<effect_type>) { ::std::type_identity<effect_type>(), #effect_type };
//#define DEFINE_EFFECT(e)

namespace code {
    using namespace shadergen::code;
    constexpr CrossEffectVariable<Vec3> modelPosition { "modelPosition" };
    constexpr CrossEffectVariable<Vec4> screenPosition { "screenPosition" };
    constexpr CrossEffectVariable<Vec3> worldPosition { "worldPosition" };
}
struct BuiltinPositionEffect {
    using Self = BuiltinPositionEffect;
    struct MatrixUniforms {
        BEGIN_GEN_STRUCT(MatrixUniforms);
        Mat4x4 GEN_FIELD(viewProjectionMatrix);
        Mat4x4 GEN_FIELD(worldViewMatrix);
        END_GEN_STRUCT();
    };
    GraphicsPtr<MatrixUniforms> matrixUniforms;
    static constexpr void defineEffect(EffectDriver& prog) {
        using namespace code;
        auto& vert = prog.vert;
        auto& frag = prog.frag;
        auto matrices = vert.uniform<&Self::matrixUniforms>("u_matrices"_v);

        auto [screenPosVert, screenPosFrag] = prog.varying<Vec4>("v_screenPos"_v);
        vert, screenPosVert = matrices.viewProjectionMatrix * compose(modelPosition, constant(1.0f));
        vert, screenPosition = screenPosVert;
        frag, screenPosition = screenPosFrag;

        auto [worldPosVert, worldPosFrag] = prog.varying<Vec3>("v_worldPos"_v);
        vert, worldPosVert = (matrices.worldViewMatrix * screenPosVert).xyz();
        vert, worldPosition = worldPosVert;
        frag, worldPosition = worldPosFrag;
    }
};

namespace code {
    constexpr CrossEffectVariable<Vec4> fragmentColor { "fragmentColor" };
}
struct BuiltinColorEffect {
    static constexpr void defineEffect(EffectDriver& prog) {
        using namespace code;
        auto& frag = prog.frag;
        auto fragColorOuput = frag.output<Vec4>("out_fragColor"_v);
        frag, fragColorOuput = fragmentColor;
    }
};

}

DEFINE_EFFECT(shadergen::effect::BuiltinPositionEffect);
DEFINE_EFFECT(shadergen::effect::BuiltinColorEffect);

namespace shadergen::effect {

struct SourceGenerator : GLSLGeneratorBase<SourceGenerator> {
    using Base = GLSLGeneratorBase<SourceGenerator>;
    SourceGenerator(PseudoVector<char> sourceBuffer)
        : Base(sourceBuffer) { }
    void emitUniformBlock(int_t binding, const UserType* type, std::string_view name = {}) {
        out() << "layout(std140, binding=" << binding << ") uniform " << type->name << " {\n";
        indent += 1;
        auto typeGenerator = GLSLTypeGenerator::fork(*this);
        typeGenerator.emitMembers(type);
        indent -= 1;
        out() << "} " << name << ";\n";
    }
};
inline void generateSourceBody(ArrayView<const EffectDescriptor*> descriptors, EffectDescriptor::StageDescriptor EffectDescriptor::*stage, SourceGenerator& generator) {
    struct DagNode {
        const EffectDescriptor* descriptor = nullptr;
        std::string debugName = {};
    };
    using DAG = DirectedAcyclicGraph<DagNode>;
    using NodeHandle = typename DAG::NodeHandle;
    struct VarInfo {
        const VarDecl* decl = nullptr;
        NodeHandle afterWrite, beforeRead;
    };
    using UsageType = CrossEffectVarUsage::UsageType;
    DAG dag;
    std::unordered_map<const VarDecl*, VarInfo> varMap;
    int_t counter = 1;
    for (const auto* effectDesc : descriptors) {
        NodeHandle effectNode = dag.newNode({ effectDesc, std::string("effect ") + std::to_string(counter++) });
        const auto& stageDesc = effectDesc->*stage;
        for (auto usage : stageDesc.ceUsages) {
            auto it = varMap.find(usage.varDecl);
            if (it == varMap.end()) {
                VarInfo varInfo { usage.varDecl, dag.newNode({ nullptr, std::string(usage.varDecl->name) + " aW" }), dag.newNode({ nullptr, std::string(usage.varDecl->name) + " bR" }) };
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
    generator.out() << "void main() {\n";
    for (const auto& pair : varMap) {
        VERIFY(ShaderGen, dag.parents(pair.second.afterWrite).size() == 1);
        generator.out() << pair.second.decl << ";\n";
    }
    dag.topologicalOrdering([&](NodeHandle nodeHandle) {
        auto& node = dag[nodeHandle];
        if (!node.descriptor)
            return;
        auto& stageDesc = node.descriptor->*stage;
        generator.out() << stageDesc.source << "\n";
    });
    generator.out() << "}";
}
struct EffectBindingInfomation {
    struct Binding {
        int_t glHandle = -1;
        int_t offsetInStruct = -1;
    };
    std::vector<Binding> uniforms;
    std::vector<Binding> attributes;
};
inline std::vector<EffectBindingInfomation> generateSource(ArrayView<const EffectDescriptor*> descriptors) {
    //for (const auto* desc : descriptors)
    //    desc->dumpAST();
    std::vector<EffectBindingInfomation> outInfos { (size_t)descriptors.size() };
    int_t uniformCounter = 0;
    {
        auto stage = &EffectDescriptor::vert;
        OversizedArray<char, 10000> buffer;
        SourceGenerator generator { buffer };
        int_t attributeCounter = 0;
        for (int_t i = 0; i < descriptors.size(); i++) {
            const auto& stageDesc = descriptors[i]->*stage;
            auto& bindings = outInfos[i];
            for (auto global : stageDesc.globals) {
                if (global.uniform) {
                    bindings.uniforms.push_back({ uniformCounter, global.offsetInStruct });
                    generator.emitUniformBlock(uniformCounter++, asUserType(global.type), global.name());
                } else if (global.input) {
                    bindings.attributes.push_back({ attributeCounter, global.offsetInStruct });
                    generator.out() << "layout(location=" << attributeCounter++ << ") in " << global.type << " " << global.name() << ";\n";
                } else if (global.output) {
                    generator.out() << "out " << global.type << " " << global.name() << ";\n";
                } else {
                    generator.out() << global.type << " " << global.name() << ";\n";
                }
            }
        }
        generateSourceBody(descriptors, stage, generator);
        VDUMP(buffer.data);
    }
    {
        auto stage = &EffectDescriptor::frag;
        OversizedArray<char, 10000> buffer;
        SourceGenerator generator { buffer };
        for (int_t i = 0; i < descriptors.size(); i++) {
            const auto& stageDesc = descriptors[i]->*stage;
            auto& bindings = outInfos[i];
            for (auto global : stageDesc.globals) {
                if (global.uniform) {
                    bindings.uniforms.push_back({ uniformCounter, global.offsetInStruct });
                    generator.emitUniformBlock(uniformCounter++, asUserType(global.type), global.name());
                } else if (global.input) {
                    generator.out() << "in " << global.type << " " << global.name() << ";\n";
                } else if (global.output) {
                    generator.out() << "out " << global.type << " " << global.name() << ";\n";
                } else {
                    generator.out() << global.type << " " << global.name() << ";\n";
                }
            }
        }
        generateSourceBody(descriptors, stage, generator);
        VDUMP(buffer.data);
    }
    return outInfos;
}
}
