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

struct ErrorFinder : ASTVisitor<ErrorFinder> {
    using Base = ASTVisitor<ErrorFinder>;
    PseudoVector<char> errorMessages;
    constexpr ErrorFinder(PseudoVector<char> errorMessages)
        : errorMessages(errorMessages) { }
    constexpr void outputMsg(std::string_view msg) {
        std::copy_n(msg.begin(), msg.size(), errorMessages.acquireEnd(msg.size()));
        errorMessages.pushBack('\0');
    }
    constexpr void visitErrorNode(const ErrorNode* err) {
        outputMsg(err->message);
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
        OversizedArray<VariableDescriptor, 100> inputs;
        OversizedArray<VariableDescriptor, 100> outputs;
        OversizedArray<CrossEffectVarUsage, 100> ceUsages;
        OversizedArray<VariableDescriptor, 100> subpassInputs;
        OversizedArray<OpenGL::SubpassInputReplacement, 100> subpassReplacements;
        OversizedArray<char, 10000> source;
        OversizedArray<char, 10000> errorMessages;

        constexpr void set(const EffectStageDriver& stage) {
            extractVariables(uniforms, stage, stage.stageDecl()->uniforms);
            extractVariables(inputs, stage, stage.stageDecl()->inputs);
            extractVariables(outputs, stage, stage.stageDecl()->outputs);
            extractVariables(subpassInputs, stage, stage.stageDecl()->subpassInputs);

            CrossEffectVarUsageFinder usageFinder { ceUsages };
            usageFinder.dispatchVisit(stage.stageDecl()->entryPoint);
            ErrorFinder errorFinder { errorMessages };
            errorFinder.dispatchVisit(stage.stageDecl()->entryPoint);

            BasicASTStorage<1000, 10000> storage;
            OpenGL::Transform transform1 { storage, stage.stageDecl() };
            GLSLStmtTransform transform2 { storage };
            OpenGL::StmtGenerator generator { source, stage.stageDecl(), subpassReplacements };
            StageDecl* transformed = transform2.createStageDecl(*transform1.createStageDecl(*stage.stageDecl()));
            generator.dispatchVisit(transformed->entryPoint);
        }
    };
    OversizedArray<const UserType*, 100> userTypes;
    StageDescriptor vert;
    StageDescriptor frag;

    using DefineFunc_t = void (*)(EffectDriver&);
    constexpr ConstexprEffectDescriptor(DefineFunc_t defineFunc) {
        EffectDriver effect;
        defineFunc(effect);
        vert.set(effect.vert);
        frag.set(effect.frag);
        UserTypeRecorder typeRecorder { userTypes };
        typeRecorder.visitStageDecl(effect.vert.stageDecl());
        typeRecorder.visitStageDecl(effect.frag.stageDecl());
    }
};

struct EffectDescriptor {
    struct VariableDescriptor {
        const Type* type = nullptr;
        std::string_view name = {};
        int_t offsetInStruct = -1;
    };
    struct VariableArray {
        ArrayView<const VariableDescriptor> view = {};
        constexpr operator ArrayView<const VariableDescriptor>() const { return view; }

        template<ConstexprEffectDescriptor::VariableDescriptor... descriptors>
        struct StaticArrayHolder {
            static inline const VariableDescriptor array[sizeof...(descriptors)] = { { descriptors.type, descriptors.name(), descriptors.offsetInStruct ? *descriptors.offsetInStruct : -1 }... };
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
    struct ErrorView {
        ArrayView<const char> messages;
        struct iterator {
            const char* position = nullptr;
            constexpr bool operator!=(const iterator& it) const {
                return position != it.position;
            }
            constexpr iterator& operator++() {
                int_t len = std::string_view(position).length();
                position += len + 1;
                return *this;
            }
            constexpr iterator operator++(int) {
                int_t len = std::string_view(position).length();
                auto copy = *this;
                position += len + 1;
                return copy;
            }
            constexpr std::string_view operator*() const {
                return { position };
            }
        };
        constexpr iterator begin() const { return { messages.begin() }; }
        constexpr iterator end() const { return { messages.end() }; }
    };
    struct StageDescriptor {
        ArrayView<const VariableDescriptor> uniforms;
        ArrayView<const VariableDescriptor> inputs;
        ArrayView<const VariableDescriptor> outputs;
        ArrayView<const CrossEffectVarUsage> ceUsages;
        ArrayView<const VariableDescriptor> subpassInputs;
        ArrayView<const OpenGL::SubpassInputReplacement> subpassReplacements;
        std::string_view source;
        ErrorView errors;

        template<ConstexprEffectDescriptor::StageDescriptor stage>
        constexpr void set() {
            uniforms = VariableArray::make<stage.uniforms>();
            inputs = VariableArray::make<stage.inputs>();
            outputs = VariableArray::make<stage.outputs>();
            ceUsages = staticView<stage.ceUsages>();
            subpassInputs = VariableArray::make<stage.subpassInputs>();
            subpassReplacements = staticView<stage.subpassReplacements>();
            ArrayView<const char> sourceView = staticView<stage.source>();
            source = { sourceView.begin(), sourceView.end() };
            errors = { staticView<stage.errorMessages>() };
        }

        void dumpErrors() const {
            if (errors.messages.size() == 0)
                return;
            log("Errors:");
            for (auto err : errors)
                log("    {}", err);
        }
    };
    ArrayView<const UserType* const> userTypes;
    StageDescriptor vert;
    StageDescriptor frag;

    template<typename E>
    constexpr EffectDescriptor(std::type_identity<E>) {
        constexpr ConstexprEffectDescriptor effect { E::defineEffect };
        userTypes = staticView<effect.userTypes>();
        vert.set<effect.vert>();
        frag.set<effect.frag>();
    }

    void dumpErrors() const {
        vert.dumpErrors();
        frag.dumpErrors();
    }
};

#define DEFINE_EFFECT(effect_type) \
    template<>                     \
    constinit const ::shadergen::effect::EffectDescriptor(::shadergen::effect::descriptorStore<effect_type>) { ::std::type_identity<effect_type>() };
#define DEFINE_BUILTIN_EFFECT(effect_type) \
    template<>                             \
    constinit const ::shadergen::effect::EffectDescriptor(descriptorStore<effect_type>) { ::std::type_identity<effect_type>() };
//#define DEFINE_EFFECT(e)
//#define DEFINE_BUILTIN_EFFECT(e)

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
DEFINE_BUILTIN_EFFECT(BuiltinPositionEffect);

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
DEFINE_BUILTIN_EFFECT(BuiltinColorEffect);

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

inline void generateSource(ArrayView<const EffectDescriptor*> descriptors) {
    for (const auto* desc : descriptors)
        desc->dumpErrors();
    int_t uniformCounter = 0;
    {
        auto stage = &EffectDescriptor::vert;
        OversizedArray<char, 10000> buffer;
        SourceGenerator generator { buffer };
        int_t attributeCounter = 0;
        for (const auto* effectDesc : descriptors) {
            const auto& stageDesc = effectDesc->*stage;
            for (auto uniform : stageDesc.uniforms)
                generator.emitUniformBlock(uniformCounter++, asUserType(uniform.type), uniform.name);
            for (auto input : stageDesc.inputs)
                generator.out() << "layout(location=" << attributeCounter++ << ") in " << input.type << " " << input.name << ";\n";
            for (auto output : stageDesc.outputs)
                generator.out() << "out " << output.type << " " << output.name << ";\n";
        }
        generateSourceBody(descriptors, stage, generator);
        VDUMP(buffer.data);
    }
    {
        auto stage = &EffectDescriptor::frag;
        OversizedArray<char, 10000> buffer;
        SourceGenerator generator { buffer };
        int_t attributeCounter = 0;
        for (const auto* effectDesc : descriptors) {
            const auto& stageDesc = effectDesc->*stage;
            for (auto uniform : stageDesc.uniforms)
                generator.emitUniformBlock(uniformCounter++, asUserType(uniform.type), uniform.name);
            for (auto input : stageDesc.inputs)
                generator.out() << "in " << input.type << " " << input.name << ";\n";
            for (auto output : stageDesc.outputs)
                generator.out() << "layout(location=0) out " << output.type << " " << output.name << ";\n";
        }
        generateSourceBody(descriptors, stage, generator);
        VDUMP(buffer.data);
    }
}
}
