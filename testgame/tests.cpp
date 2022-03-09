#include "graphics/shadergen/ASTDumper.hpp"
#include "graphics/shadergen/BuildDriver.hpp"
#include "graphics/shadergen/GLSLGenerator.hpp"
#include "util/log.hpp"

using namespace shadergen;
using namespace shadergen::code;
// using namespace shadergen::compressed;

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
    auto foo = ast.declareScalar("foo", &FloatType);
    auto bar = ast.declareVector("bar", FloatType.vectorType(4));
    auto baz = ast.declareVector("baz", FloatType.vectorType(4));
    auto a = ast.variable<A>("a");
    auto res = ast.declareVector("res", BoolType.vectorType(4));

    ast.block([&]() {
        ast, a.b1.i = constant(1);
        ast, a.b1.f = constant(2.5f);
        ast, a.f = constant(3.0f);
        ast, foo = a.b1.f;
        ast, bar.xz() = compose(scale, constant(1.0f));
        ast, bar.yw() = compose(scale, constant(2.0f));
        ast, bar.x = floor(foo);
        ast, baz = compose(foo, bar.xy(), foo);
        ast, res = !(foo * foo + bar < bar + baz) || constant(true, false, false, true);
        ast.branch(
            all(res),
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

    auto varColor = prog.varying<Vec3>("v_color");

    auto posAttr = vert.attribute<Vec2>("a_position");
    auto colorAttr = vert.attribute<Vec3>("a_color");
    vert, vert.position() = compose(posAttr, constant(0.0f, 1.0f));
    vert, varColor = colorAttr;

    auto fragColor = frag.colorOutput<Vec4>("fragColor");
    auto multiplier = frag.uniform<float>("u_multiplier");
    frag, fragColor = compose(multiplier * varColor, constant(1.0f));
}

static constexpr auto toGLSL(const GraphicsDriver& prog) {
    std::array<char, 10000> buffer = {};
    GLSLStageGenerator glsl { buffer.data() };
    glsl.visitStage(prog.vert);
    return buffer;
}

void runShadergenTest() {
    ASTDumper dumper;

    // static constexpr auto tuple = createTuple<createAST>();
    // dumper.dump(tuple.root);

    static constexpr auto ast = GraphicsDriver(createShader);
    dumper.dump(ast.vert.mainBlock);
    static constexpr auto glsl = toGLSL(ast);
    log("{}", glsl.data());

    /*A aVal;
    aVal.b2.i = 123;
    auto a = reflect(aVal);
    log("leafFieldCount: {}", a.leafFieldCount());
    log("test: {}", a.nameOf(a.b2.d).view());*/
}
