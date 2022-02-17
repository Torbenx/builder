#include "graphics/shadergen/shadergen.hpp"
#include "util/log.hpp"

using namespace shadergen;
using namespace shadergen::code;

static auto constexprTest() {
    StmtConsumer<StmtDumper, Checker, PersistentAST> consumer;
    auto foo = consumer.declareVariable<"foo">(makeVectorType<float, 1>());
    auto bar = consumer.declareVariable<"bar">(makeVectorType<float, 4>());
    auto baz = consumer.declareVariable<"baz">(makeVectorType<float, 4>());
    auto res = consumer.declareVariable<"res">(makeVectorType<bool, 4>());
    consumer.beginBlock();
    consumer, foo = LiteralExpr(1.0f);
    consumer, bar = LiteralExpr(vec<float, 4>());
    consumer, baz = bar;
    consumer, res = !(foo * foo + bar < baz + bar);
    consumer.endBlock();

    PersistentAST& ast = std::get<PersistentAST>(consumer.consumers);
    PersistentASTDumper dumper(ast);
    dumper.dump(ast.currentBlock);
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
