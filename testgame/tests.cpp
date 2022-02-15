#include "graphics/shadergen/shadergen.hpp"
#include "util/log.hpp"

using namespace shadergen;
using namespace shadergen::code;

static PersistentAST constexprTest() {
    auto foo = makeVariable<"foo">(makeVectorType<float, 1>());
    auto bar = makeVariable<"bar">(makeVectorType<float, 4>());
    auto baz = makeVariable<"baz">(makeVectorType<float, 4>());
    PersistentAST ast;
    PersistentAST::Builder builder(ast);
    builder.dispatchVisit(foo + foo + foo + bar);
    ASTDumper dumper;
    dumper.dispatchVisit(!(foo * foo + bar < baz + bar));
    log("vars: {}", ast.varContext.variables.size());
    return ast;
}

struct A {
    struct B {
        BEGIN_REF_STRUCT(B);
        int REF_FIELD(i);
        double REF_FIELD(d);
        END_REF_STRUCT();
    };
    BEGIN_REF_STRUCT(A);
    B REF_FIELD(b1);
    B REF_FIELD(b2);
    END_REF_STRUCT();
};

void runShadergenTest() {
    PersistentAST ast = constexprTest();
    A aVal;
    aVal.b2.i = 123;
    auto a = reflect(aVal);
    log("leafFieldCount: {}", a.leafFieldCount());
    log("test: {}", a.b2.i(aVal));
}
