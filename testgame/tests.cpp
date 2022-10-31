
//#include "graphics/render3d/EffectDescriptor.hpp"
#include "graphics/shadergen/ASTDumper.hpp"
#include "graphics/shadergen/BuildDriver.hpp"
#include "graphics/shadergen/CompressedAST.hpp"
#include "graphics/shadergen/GLSLGenerator.hpp"
#include "graphics/shadergen/compute.hpp"
#include "util/log.hpp"

using namespace shadergen;
using namespace shadergen::code;

struct A {
    struct B {
        BEGIN_GEN_STRUCT(B);
        unsigned GEN_FIELD(i);
        float GEN_FIELD(f);
        END_GEN_STRUCT();
    };
    BEGIN_GEN_STRUCT(A);
    B GEN_FIELD(b1);
    B GEN_FIELD(b2);
    float GEN_FIELD(f);
    END_GEN_STRUCT();
};

static constexpr void createAST(ComputeDriver& ast) {
    auto scale = ast.variable<float>("scale"_v);
    auto foo = ast.variable<float>("foo"_v);
    auto bar = ast.variable<Vec4>("bar"_v);
    auto baz = ast.variable<Vec4>("baz"_v);
    auto ua = ast.uniformBuffer<A>("u_a"_v);
    auto a = ast.variable<A>("a"_v);
    auto res = ast.variable<bool>("res"_v);
    auto image = ast.storageImage<uImage2D>("u_image", MemQual::CanAlias);
    auto buffer = ast.storageBuffer<float>("u_buffer", MemQual::NoWrite);

    ast.block([&]() {
        ast, a = ua;
        ast, a.b1.i = imageLoad(image, constant(1, 2)).x;
        ast, a.b1.f = buffer[constant(1)];
        ast, a.f = constant(3.0f);
        ast, foo = (-bar).x;
        ast, bar.xz() = compose(scale, constant(1.0f));
        ast, bar.yw() = compose(scale, constant(2.0f));
        ast, bar.x = floor(ua.b1.f);
        ast, baz = compose(foo, bar.xy(), foo);
        ast, res = all(!(foo * (foo + bar) < bar + baz) || constant(true, false, false, true));
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

struct UiData {
    BEGIN_GEN_STRUCT(UiData);
    int GEN_FIELD(maxLayers);
    END_GEN_STRUCT();
};
static constexpr void createUiShader(ComputeDriver& prog) {
    auto data = prog.uniformBuffer<UiData>("data"_v);
    auto positions = prog.storageBuffer<iVec2>("positions"_v, MemQual::NoWrite);
    auto screen = prog.storageImage<uImage2D>("screen"_v, (MemQual)0);
    auto atlas = prog.storageImage<uImage2D>("atlas"_v, MemQual::NoWrite);

    AUTO_VARIABLE(prog, offset) = prog.globalIndex.x * (data.maxLayers + constant(1));
    AUTO_VARIABLE(prog, screenPos) = positions[offset] + prog.localIndex.xy();
    AUTO_VARIABLE(prog, color) = imageLoad(screen, screenPos).xyz();
    prog.range(constant(0), data.maxLayers, [&](auto& i) {
        AUTO_VARIABLE(prog, atlasPos) = positions[i + constant(1)] + prog.localIndex.xy();
        AUTO_VARIABLE(prog, inColor) = imageLoad(atlas, atlasPos);
        prog, color = (inColor.xyz() * inColor.w + color * (constant(255u) - inColor.w)) / constant(255u);
    });
    prog, imageStore(screen, screenPos, compose(color, constant(255u)));
}

/*constexpr shadergen::effect::CrossEffectVariable<float> myVar { "myVar" };

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
DEFINE_EFFECT(Reflection)*/

void runShadergenTest() {
    ASTDumper dumper;

    static constexpr Shader shader = generate<createUiShader>();
    dumper.dump(shader.shaderDecl);
    for (Stage stage : shader.stages) {
    log("-----------------------------------");
        log("{}", stage.source);
    }

    // static constexpr auto astTransformed = transformAST(ast);
    // static constexpr auto glsl = toGLSL(astTransformed);
    // log("{}", glsl.data);

    /*const shadergen::effect::EffectDescriptor* handles[] = {
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
