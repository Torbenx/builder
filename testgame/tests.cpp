
#include "graphics/render3d/EffectDescriptor.hpp"
#include "graphics/shadergen/ASTDumper.hpp"
#include "graphics/shadergen/BuildDriver.hpp"
#include "graphics/shadergen/CompressedAST.hpp"
#include "graphics/shadergen/GLSLGenerator.hpp"
#include "util/log.hpp"

using namespace shadergen;
using namespace shadergen::code;

struct A {
    struct B {
        BEGIN_GEN_STRUCT(B);
        int GEN_FIELD(i);
        float GEN_FIELD(f);
        END_GEN_STRUCT();
    };
    BEGIN_GEN_STRUCT(A);
    B GEN_FIELD(b1);
    B GEN_FIELD(b2);
    float GEN_FIELD(f);
    END_GEN_STRUCT();
};

static constexpr void createAST(GraphicsDriver& prog) {
    auto& ast = prog.vert;
    auto scale = ast.uniform<float>("scale"_v);
    auto foo = ast.variable<float>("foo"_v);
    auto bar = ast.variable<Vec4>("bar"_v);
    auto baz = ast.variable<Vec4>("baz"_v);
    auto a = ast.variable<A>("a"_v);
    auto res = ast.variable<bool>("res"_v);

    ast.block([&]() {
        ast, a.b1.i = constant(1);
        ast, a.b1.f = constant(2.5f);
        ast, a.f = constant(3.0f);
        ast, foo = a.b1.i;
        ast, bar.xz() = compose(scale, constant(1.0f));
        ast, bar.yw() = compose(scale, constant(2.0f));
        ast, bar.x = floor(a.b1.i);
        ast, baz = compose(foo, bar.xy(), foo);
        ast, res = all(!(foo * foo * bar < bar + baz) || constant(true, false, false, true));
        ast.branch(
            res,
            [&]() {
                ast, foo = constant(1.5f);
            },
            [&]() {
                ast, foo -= constant(1.3f);
            });
    });
}

static constexpr auto toGLSL(const GraphicsDriver& prog) {
    shadergen::effect::OversizedArray<char, 10000> buffer = {};
    GLSLStageGenerator glsl { buffer };
    glsl.visitStage(prog.vert);
    glsl.out() << "\n------------------\n";
    glsl.visitStage(prog.frag);
    return buffer;
}

static constexpr auto transformAST(const GraphicsDriver& in) {
    return GraphicsDriver([&](GraphicsDriver& out) {
        GLSLStmtTransform transform { out.storage };
        out.vert.m_stageDecl = transform.createStageDecl(*in.vert.stageDecl());
        out.frag.m_stageDecl = transform.createStageDecl(*in.frag.stageDecl());
    });
}

constexpr shadergen::effect::CrossEffectVariable<float> myVar { "myVar" };

struct PositionInput {
    GraphicsPtr<Vec3[]> positionAttribute;

    static constexpr void defineEffect(shadergen::effect::EffectDriver& prog) {
        using namespace shadergen::effect::code;
        prog.vert, modelPosition = prog.vert.input<&PositionInput::positionAttribute>("a_position"_v);
    }
};
DEFINE_EFFECT(PositionInput)

struct Reflection {
    struct ParameterUniform {
        BEGIN_GEN_STRUCT(ParameterUniform)
        float GEN_FIELD(factor);
        Vec3 GEN_FIELD(baseColor);
        END_GEN_STRUCT()
    };
    GraphicsPtr<ParameterUniform> paramUniform;
    GraphicsPtr<Vec4> subpass;
    static constexpr void defineEffect(shadergen::effect::EffectDriver& prog) {
        using namespace shadergen::effect::code;
        auto params = prog.frag.uniform<&Reflection::paramUniform>("u_params"_v);
        auto subpassInput = prog.frag.subpassInput<&Reflection::subpass>("i_subpass"_v);
        prog.frag, fragmentColor = compose(params.baseColor + subpassInput.xyz(), constant(1.0f));
    }
};
DEFINE_EFFECT(Reflection)

void runShadergenTest() {
    ASTDumper dumper;

    static constexpr auto ast = GraphicsDriver(createAST);
    static constexpr const StageDecl* astCompressed = compressStage<ast.vert.stageDecl()>();
    dumper.dump(ast.vert.stageDecl());
    log("-----------------------------------");
    dumper.dump(astCompressed);
    const auto& compressed = compressed::CompressedStageHolder<ast.vert.stageDecl()>::compressedStage;
    VDUMP(sizeof(compressed));
    VDUMP(sizeof(ast));
    static constexpr auto astTransformed = transformAST(ast);
    static constexpr auto glsl = toGLSL(astTransformed);
    log("{}", glsl.data);

    const shadergen::effect::EffectDescriptor* handles[] = {
        &shadergen::effect::descriptorStore<shadergen::effect::BuiltinPositionEffect>,
        &shadergen::effect::descriptorStore<shadergen::effect::BuiltinColorEffect>,
        &shadergen::effect::descriptorStore<PositionInput>,
        &shadergen::effect::descriptorStore<Reflection>,
    };
    shadergen::effect::generateSource(handles);
    /*static constexpr shadergen::effect::ConstexprEffectDescriptor desc { Effect1::defineEffect };
    shadergen::effect::EffectDescriptor::StageDescriptor stage;
    stage.set<desc.vert>();
    for (auto var : desc.vert.uniforms)
        VDUMP(var.name());
    for (auto var : desc.vert.subpassInputs)
        VDUMP(var.name());*/
}
