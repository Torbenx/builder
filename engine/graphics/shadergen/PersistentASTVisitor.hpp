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
        impl()->dispatchVisit(e.left);
        impl()->dispatchVisit(e.right);
        return impl()->visitExpr(e);
    }
    constexpr RetT visitVectorByScalarBinaryOperatorExpr(const VectorByScalarBinaryOperatorExprNode& e) {
        impl()->dispatchVisit(e.vector);
        impl()->dispatchVisit(e.scalar);
        return impl()->visitExpr(e);
    }
    constexpr RetT visitUnaryOperatorExpr(const UnaryOperatorExprNode& e) {
        impl()->dispatchVisit(e.subExpr);
        return impl()->visitExpr(e);
    }
    constexpr RetT visitAssignmentStmt(const AssignmentStmtNode& e) {
        impl()->dispatchVisit(e.left);
        impl()->dispatchVisit(e.right);
        return impl()->visitStmt(e);
    }
    constexpr RetT visitLiteralExpr(const LiteralExprNode& e) {
        return impl()->visitExpr(e);
    }
    constexpr RetT visitVarDeclStmt(const VarDeclStmtNode& e) {
        return impl()->visitStmt(e);
    }
    constexpr RetT visitCompoundStmt(const CompoundStmtNode& e) {
        e.forEachStmt(ast, [&](NodeHandle<StmtNode> handle) {
            impl()->dispatchVisit(handle);
        });
        return impl()->visitStmt(e);
    }
    constexpr RetT visitBlockStmt(const BlockStmtNode& e) {
        return impl()->visitCompoundStmt(e);
    }
    constexpr RetT visitFieldAccessExpr(const FieldAccessExprNode& e) {
        impl()->dispatchVisit(e.base);
        return impl()->visitExpr(e);
    }

    constexpr const TypeInfo& getType(const TypeHandle& type) {
        return ast.typeContext.type(type);
    }
    constexpr const VarDeclStmtNode& getVarDecl(const VarExprNode& e) {
        return ast.node(e.varDecl);
    }

    // dispatch
    constexpr RetT dispatchVisit(NodeHandle<StmtNode> handle) {
        const NodeStorage& storage = ast.nodeStorage(handle);
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
                else if constexpr (std::is_same_v<T, VarDeclStmtNode>)
                    return impl()->visitVarDeclStmt(node);
                else if constexpr (std::is_same_v<T, CompoundStmtNode>)
                    return impl()->visitCompoundStmt(node);
                else if constexpr (std::is_same_v<T, BlockStmtNode>)
                    return impl()->visitBlockStmt(node);
                else if constexpr (std::is_same_v<T, FieldAccessExprNode>)
                    return impl()->visitFieldAccessExpr(node);
                else
                    VERIFY_NOT_REACHED(ShaderGen);
            },
            storage);
    }
};
}
