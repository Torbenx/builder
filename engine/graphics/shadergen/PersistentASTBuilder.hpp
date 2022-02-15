#pragma once

#include "CheckedAST.hpp"

namespace shadergen {

struct PersistentAST::Builder : CheckedASTVisitor<Builder, NodeHandle<ExprNode>> {
    PersistentAST& outAst;
    constexpr Builder(PersistentAST& outAst)
        : outAst(outAst) { }
    constexpr Builder(PersistentAST& inAst, PersistentAST& outAst)
        : CheckedASTVisitor<Builder, NodeHandle<ExprNode>>(inAst), outAst(outAst) { }
    constexpr TypeContext& typeContext() { return outAst.typeContext; }
    constexpr VarDeclContext& varDeclContext() { return outAst.varContext; }
    constexpr ExprNode buildExpr(auto e) {
        return { e.type() };
    }
    constexpr NodeHandle<ExprNode> visitExpr(auto e) {
        return outAst.emit(buildExpr(e));
    }
    constexpr NodeHandle<ExprNode> visitVarExpr(auto e) {
        return outAst.emit(VarExprNode { buildExpr(e), e.varDecl() });
    }
    constexpr NodeHandle<ExprNode> visitElementwiseBinaryOperatorExpr(auto e) {
        return outAst.emit(ElementwiseBinaryOperatorExprNode {
            buildExpr(e),
            e.op(),
            dispatchVisit(e.left()),
            dispatchVisit(e.right()) });
    }
    constexpr NodeHandle<ExprNode> visitVectorByScalarBinaryOperatorExpr(auto e) {
        return outAst.emit(VectorByScalarBinaryOperatorExprNode {
            buildExpr(e),
            e.op(),
            dispatchVisit(e.vector()),
            dispatchVisit(e.scalar()),
            e.isScalarLeft() });
    }
    constexpr NodeHandle<ExprNode> visitUnaryOperatorExpr(auto e) {
        return outAst.emit(UnaryOperatorExprNode {
            buildExpr(e),
            e.op(),
            dispatchVisit(e.subExpr()) });
    }
};

}
