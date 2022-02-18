#include "graphics/shadergen2/AST.hpp"
#include "util/log.hpp"
#include "util/reflection.hpp"

using namespace shadergen;
using namespace shadergen::code;

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
    float REF_FIELD(f);
    END_REF_STRUCT();
};

/*static auto constexprTest() {
    StmtConsumer<Checker, PersistentAST, StmtDumper> consumer;
    auto a = consumer.declareVariable<A, "a">();
    auto foo = consumer.declareVariable<float, "foo">();
    auto bar = consumer.declareVariable<"bar">(makeVectorType<float, 4>());
    auto baz = consumer.declareVariable<"baz">(makeVectorType<float, 4>());
    auto res = consumer.declareVariable<"res">(makeVectorType<bool, 4>());
    consumer.beginBlock();
    consumer, foo = a.f;
    consumer, foo = wrap(LiteralExpr(1.0f));
    consumer, bar = wrap(LiteralExpr(vec<float, 4>()));
    consumer, baz = bar;
    consumer, res = !(foo * foo + bar < baz + bar);
    consumer.endBlock();

    return std::get<PersistentAST>(consumer.consumers);
}*/
static constexpr void constexprTest(WrappedAST& ast) {
    auto foo = ast.declareScalar("foo", &FloatType);
    auto bar = ast.declareVector("bar", FloatType.vectorType(4));
    auto baz = ast.declareVector("baz", FloatType.vectorType(4));
    auto res = ast.declareVector("res", BoolType.vectorType(4));
    auto lit1 = wrapped<LiteralExpr, void>({ {} });
    lit1.type = &FloatType;
    auto lit2 = wrapped<LiteralExpr, vec_t>({ {} });
    lit2.type = FloatType.vectorType(4);
    ast.emitStmt(foo = lit1);
    ast.emitStmt(bar = lit2);
    ast.emitStmt(baz = bar);
    ast.emitStmt(res = !(foo * foo + bar < baz + bar));
}

void runShadergenTest() {
    //PersistentAST ast = constexprTest();
    //PersistentASTDumper dumper(ast);
    //dumper.dump(ast.currentBlock);
    static constexpr auto ast = WrappedAST(constexprTest);

    A aVal;
    aVal.b2.i = 123;
    auto a = reflect(aVal);
    log("leafFieldCount: {}", a.leafFieldCount());
    log("test: {}", a.nameOf(a.b2.d).view());
}
