#include "graphics/shadergen/shadergen.hpp"
#include "util/log.hpp"

using namespace shadergen;
using namespace shadergen::code;

static constexpr auto constexprTest() {
    PersistentAST ast;

    auto emit = [&](auto stmt) {
        TemplateASTDumper dumper0;
        dumper0.dispatchVisit(stmt);

        PersistentAST::Builder builder(ast);
        auto node = ast.emit(builder.dispatchVisit(stmt));
        PersistentASTDumper dumper1(ast);
        dumper1.dispatchVisit(node);
    };
    auto foo = ast.varContext.declareVariable<"foo">(makeVectorType<float, 4>());
    auto bar = ast.varContext.declareVariable<"bar">(makeVectorType<float, 4>());
    auto baz = ast.varContext.declareVariable<"baz">(makeVectorType<float, 4>());
    auto res = ast.varContext.declareVariable<"res">(makeVectorType<bool, 4>());

    emit(foo = foo);
    emit(bar = foo);
    emit(baz = bar);
    emit(res = !(foo * foo + bar < baz + bar));
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
