#include "graphics/shadergen/shadergen.hpp"
#include "util/log.hpp"

using namespace shadergen;
using namespace shadergen::code;

static void constexprTest() {
    auto foo = makeVariable<"foo">(makeVectorType<float, 1>());
    auto bar = makeVariable<"bar">(makeVectorType<float, 4>());
    auto baz = makeVariable<"baz">(makeVectorType<float, 4>());
    auto haha = makeVariable<"haha">(makeVectorType<float, 1>());

    PersistentAST ast;
    PersistentAST::Builder builder(ast);
    ASTDumper dumper(ast);

    auto expr = !(foo * foo + bar < baz + bar);
    dumper.dispatchVisit(expr);

    builder.dispatchVisit(haha);
    auto node = builder.dispatchVisit(expr);
    dumper.dispatchVisit(node);

    PersistentAST ast2;
    PersistentAST::Builder builder2(ast, ast2);
    ASTDumper dumper2(ast2);

    auto node2 = builder2.dispatchVisit(node);
    dumper2.dispatchVisit(node2);
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
    constexprTest();
    A aVal;
    aVal.b2.i = 123;
    auto a = reflect(aVal);
    log("leafFieldCount: {}", a.leafFieldCount());
    log("test: {}", a.nameOf(a.b2.d).view());
}
