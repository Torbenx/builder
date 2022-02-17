#pragma once

#include "PersistentAST.hpp"

namespace shadergen {

template<typename Impl, typename RetT = void>
struct PersistentASTVisitor {
    const PersistentAST& ast;
    constexpr PersistentASTVisitor(const PersistentAST& ast)
        : ast(ast) { }

    constexpr Impl* impl() { return static_cast<Impl*>(this); }

    constexpr RetT visitStmt(const StmtNode&) {
        return void();
    }
    constexpr RetT visitExpr(const ExprNode& e) {
        return impl()->visitStmt(e);
    }
    constexpr RetT visitVarExpr(const VarExprNode& e) {
        return impl()->visitExpr(e);
    }
    constexpr RetT visitElementwiseBinaryOperatorExpr(const ElementwiseBinaryOperatorExprNode& e) {
        dispatchVisit(e.left);
        dispatchVisit(e.right);
        return impl()->visitExpr(e);
    }
    constexpr RetT visitVectorByScalarBinaryOperatorExpr(const VectorByScalarBinaryOperatorExprNode& e) {
        dispatchVisit(e.vector);
        dispatchVisit(e.scalar);
        return impl()->visitExpr(e);
    }
    constexpr RetT visitUnaryOperatorExpr(const UnaryOperatorExprNode& e) {
        dispatchVisit(e.subExpr);
        return impl()->visitExpr(e);
    }
    constexpr RetT visitAssignmentStmt(const AssignmentStmtNode& e) {
        dispatchVisit(e.left);
        dispatchVisit(e.right);
        return impl()->visitStmt(e);
    }
    constexpr auto visitLiteralExpr(const LiteralExprNode& e) {
        return impl()->visitExpr(e);
    }

    constexpr const TypeInfo& getType(const ExprNode& e) {
        return ast.typeContext.type(e.type);
    }
    constexpr const VarDecl& getVarDecl(const VarExprNode& e) {
        return ast.varContext.varDecl(e.varDecl);
    }

    // dispatch
    constexpr RetT dispatchVisit(NodeHandle<StmtNode> handle) {
        const NodeStorage& storage = ast.node(handle);
        return std::visit(
            [&]<typename T>(const T& node) -> RetT {
                if constexpr (std::is_same_v<T, ExprNode>)
                    return impl()->visitExpr(node);
                else if constexpr (std::is_same_v<T, VarExprNode>)
                    return impl()->visitVarExpr(node);
                else if constexpr (std::is_same_v<T, ElementwiseBinaryOperatorExprNode>)
                    return impl()->visitElementwiseBinaryOperatorExpr(node);
                else if constexpr (std::is_same_v<T, VectorByScalarBinaryOperatorExprNode>)
                    return impl()->visitVectorByScalarBinaryOperatorExpr(node);
                else if constexpr (std::is_same_v<T, UnaryOperatorExprNode>)
                    return impl()->visitUnaryOperatorExpr(node);
                else if constexpr (std::is_same_v<T, AssignmentStmtNode>)
                    return impl()->visitAssignmentStmt(node);
                else if constexpr (std::is_same_v<T, LiteralExprNode>)
                    return impl()->visitLiteralExpr(node);
                else
                    VERIFY_NOT_REACHED(ShaderGen);
            },
            storage);
    }
};
}
