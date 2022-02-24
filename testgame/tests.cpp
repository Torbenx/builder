#include "graphics/shadergen/AST.hpp"
#include "util/log.hpp"
//#include "util/reflection.hpp"

using namespace shadergen;
using namespace shadergen::code;
// using namespace shadergen::compressed;

struct A {
    struct B {
        BEGIN_GEN_STRUCT(B);
        int GEN_FIELD(i);
        bool GEN_FIELD(b);
        END_GEN_STRUCT();
    };
    BEGIN_GEN_STRUCT(A);
    B GEN_FIELD(b1);
    B GEN_FIELD(b2);
    float GEN_FIELD(f);
    END_GEN_STRUCT();
};

static constexpr void createAST(BuildDriver& ast) {
    ast.beginBlock();
    auto foo = ast.declareScalar("foo", &FloatType);
    auto bar = ast.declareVector("bar", FloatType.vectorType(4));
    auto baz = ast.declareVector("baz", FloatType.vectorType(4));
    auto a = ast.declareVariable<A>("a");
    auto res = ast.declareVector("res", BoolType.vectorType(4));

    ast.beginBlock();
    auto lit = constant(1.0f, 2.0f);
    ast, foo = a.b1.i;
    ast, bar.xz() = lit;
    ast, bar.yw() = lit;
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
    ast.endBlock();

    ast.endBlock();
}

void runShadergenTest() {
    ASTDumper dumper;

    // static constexpr auto tuple = createTuple<createAST>();
    // dumper.dump(tuple.root);

    static constexpr auto ast = BuildDriver(createAST);
    dumper.dump(ast.builder.currentBlock);

    /*A aVal;
    aVal.b2.i = 123;
    auto a = reflect(aVal);
    log("leafFieldCount: {}", a.leafFieldCount());
    log("test: {}", a.nameOf(a.b2.d).view());*/
}
