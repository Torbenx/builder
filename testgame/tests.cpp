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
    dumper.dispatchVisit(!(foo * bar < baz + bar));
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
}
