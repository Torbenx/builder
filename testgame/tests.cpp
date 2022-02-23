#include "graphics/shadergen/AST.hpp"
#include "util/log.hpp"
//#include "util/reflection.hpp"

using namespace shadergen;
using namespace shadergen::code;
//using namespace shadergen::compressed;

/*struct A {
    struct B {
        BEGIN_REF_STRUCT(B);
        int REF_FIELD(i);
        double REF_FIELD(d);
        END_REF_STRUCT();
    };
    BEGIN_REF_STRUCT(A);
    B REF_FIELD(b1);
    B REF_FIELD(b2);
    float REF_FIELD(f);
    END_REF_STRUCT();
};*/

static constexpr void createAST(BuildDriver& ast) {
    ast.beginBlock();
    auto foo = ast.declareScalar("foo", &FloatType);
    auto bar = ast.declareVector("bar", FloatType.vectorType(4));
    auto baz = ast.declareVector("baz", FloatType.vectorType(4));
    auto res = ast.declareVector("res", BoolType.vectorType(4));

    ast.beginBlock();
    auto lit = constant(1.0f, 2.0f);
    ast, foo = constant(1.0f);
    ast, bar.xz() = lit;
    ast, bar.yw() = lit;
    ast, bar.x = floor(foo);
    ast, baz = compose(foo, bar.xy(), foo);
    ast, res = !(foo * foo + bar < bar + baz) || constant(true, false, false, true);
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
