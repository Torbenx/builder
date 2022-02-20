#include "graphics/shadergen/CompressedAST.hpp"
#include "util/log.hpp"
//#include "util/reflection.hpp"

using namespace shadergen;
using namespace shadergen::code;
using namespace shadergen::compressed;

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

static constexpr void createAST(WrappedAST& ast) {
    ast.beginBlock();
    auto foo = ast.declareScalar("foo", &FloatType);
    auto bar = ast.declareVector("bar", FloatType.vectorType(4));
    auto baz = ast.declareVector("baz", FloatType.vectorType(4));
    auto res = ast.declareVector("res", BoolType.vectorType(4));

    ast.beginBlock();
    auto lit1 = wrapped<LiteralExpr, void>({ {} });
    lit1.type = &FloatType;
    auto lit2 = wrapped<LiteralExpr, vec_t>({ {} });
    lit2.type = FloatType.vectorType(2);
    ast, foo = lit1;
    ast, bar.xz() = lit2;
    ast, bar.yw() = lit2;
    ast, baz = bar;
    ast, res = !(foo * foo + bar < bar + baz);
    ast, res = res;
    ast.endBlock();

    ast.endBlock();
}

void runShadergenTest() {
    static constexpr auto tuple = createTuple<createAST>();
    ASTDumper dumper;
    dumper.dump(WrappedAST(createAST).currentBlock);

    /*A aVal;
    aVal.b2.i = 123;
    auto a = reflect(aVal);
    log("leafFieldCount: {}", a.leafFieldCount());
    log("test: {}", a.nameOf(a.b2.d).view());*/
}
