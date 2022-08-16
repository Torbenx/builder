#include "graphics/shadergen/ASTDumper.hpp"
#include "graphics/shadergen/BuildDriver.hpp"
#include "graphics/shadergen/GLSLGenerator.hpp"
#include "util/log.hpp"

#include "graphics/render3d/EffectDescriptor.hpp"

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
    auto scale = ast.uniform<float>("scale");
    auto foo = ast.variable<float>("foo");
    auto bar = ast.variable<Vec4>("bar");
    auto baz = ast.variable<Vec4>("baz");
    auto a = ast.variable<A>("a");
    auto res = ast.variable<bool>("res");

    ast.block([&]() {
        ast, a.b1.i = constant(1);
        ast, a.b1.f = constant(2.5f);
        ast, a.f = constant(3.0f);
        ast, foo = a.b1.f;
        ast, bar.xz() = compose(scale, constant(1.0f));
        ast, bar.yw() = compose(scale, constant(2.0f));
        ast, bar.x = floor(foo);
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

static constexpr void createShader(GraphicsDriver& prog) {
    auto& vert = prog.vert;
    auto& frag = prog.frag;

    auto varPos = prog.varying<Vec2>("v_texCoord");

    auto posAttr = vert.input<Vec2>("a_position");
    auto trans = vert.uniform<Mat4x4>("u_trans");
    vert, vert.position = trans * compose(posAttr, constant(0.0f, 1.0f)) ;
    vert, varPos.in = posAttr;

    auto fragColor = frag.output<Vec4>("fragColor");
    auto multiplier = frag.uniform<float>("u_multiplier");
    auto tex = frag.uniform<Sampler2D>("u_texture");
    frag, fragColor = multiplier * texture(tex, varPos.out);
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

struct Effect1 {
    GraphicsPtr<Vec3[]> attribute;
    struct VertexUniform {
        BEGIN_GEN_STRUCT(VertexUniform)
        Vec3 GEN_FIELD(position);
        END_GEN_STRUCT()
    };
    GraphicsPtr<VertexUniform> vertexUniform;

    static constexpr void defineEffect(shadergen::effect::EffectDriver& prog) {
        using namespace shadergen::code;
        auto attr = prog.vert.uniform<&Effect1::vertexUniform>("a_attr");
        auto subpass = prog.vert.subpassInput<&Effect1::attribute>("i_subpass");
        prog.vert, attr.position += compose(constant(1.0f), subpass.w, myVar);
    }
};
DEFINE_EFFECT(Effect1)
struct Effect2 {
    GraphicsPtr<float[]> attribute;
    static constexpr void defineEffect(shadergen::effect::EffectDriver& prog) {
        using namespace shadergen::code;
        auto attr = prog.vert.uniform<&Effect2::attribute>("a_attr");
        prog.vert, myVar = attr;
    }
};
DEFINE_EFFECT(Effect2)
struct Effect3 {
    GraphicsPtr<float[]> attribute;
    static constexpr void defineEffect(shadergen::effect::EffectDriver& prog) {
        using namespace shadergen::code;
        auto attr = prog.vert.uniform<&Effect3::attribute>("a_attr");
        prog.vert, myVar = myVar + attr;
    }
};
DEFINE_EFFECT(Effect3)

void runShadergenTest() {
    ASTDumper dumper;

    // static constexpr auto tuple = createTuple<createAST>();
    // dumper.dump(tuple.root);

    static constexpr auto ast = GraphicsDriver(createShader);
    dumper.dump(ast.vert.stageDecl()->entryPoint);
    dumper.dump(ast.frag.stageDecl()->entryPoint);
    static constexpr auto ast2 = transformAST(ast);
    static constexpr auto glsl2 = toGLSL(ast2);
    log("{}", glsl2.data);

    const shadergen::effect::EffectDescriptor* handles[] = { &shadergen::effect::descriptorStore<Effect1>, &shadergen::effect::descriptorStore<Effect2>, &shadergen::effect::descriptorStore<Effect3> };
    shadergen::effect::generateSource(handles);
    VDUMP(shadergen::effect::EffectHandle((Effect1*)nullptr).descriptor->vert.source);
    shadergen::effect::BuiltinPositionEffect* bpe = nullptr;
    VDUMP(shadergen::effect::EffectHandle(bpe).descriptor->vert.source);
    VDUMP(shadergen::effect::EffectHandle(bpe).descriptor->frag.source);
}
