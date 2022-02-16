#pragma once

#include "PersistentASTVisitor.hpp"

namespace shadergen {

struct PersistentAST::CheckExprUsage : PersistentASTVisitor<CheckExprUsage> {
    constexpr CheckExprUsage(PersistentAST& ast)
        : PersistentASTVisitor<CheckExprUsage>(ast) { }

    constexpr void visitVarExpr(const VarExprNode& e) {
        VERIFY(ShaderGen, getVarDecl(e).isInitialized);
    }
};

struct PersistentAST::Builder : TemplateASTVisitor<Builder> {
    PersistentAST& ast;
    constexpr Builder(PersistentAST& ast)
        : ast(ast) { }

    constexpr ExprNode visitExpr(auto e) {
        return { {}, ast.typeContext.findType(e.type()) };
    }

    constexpr VarExprNode visitVarExpr(auto e) {
        VERIFY(ShaderGen, ast.varContext.varDecl(e.varDecl).name() == e.name());
        return { visitExpr(e), e.varDecl };
    }

    constexpr ElementwiseBinaryOperatorExprNode visitElementwiseBinaryOperatorExpr(auto e) {
        return {
            visitExpr(e),
            e.op(),
            ast.emit(dispatchVisit(e.left)),
            ast.emit(dispatchVisit(e.right))
        };
    }

    constexpr VectorByScalarBinaryOperatorExprNode visitVectorByScalarBinaryOperatorExpr(auto e) {
        return {
            visitExpr(e),
            e.op(),
            ast.emit(dispatchVisit(e.vector)),
            ast.emit(dispatchVisit(e.scalar)),
            e.isScalarLeft()
        };
    }

    constexpr UnaryOperatorExprNode visitUnaryOperatorExpr(auto e) {
        return {
            visitExpr(e),
            e.op(),
            ast.emit(dispatchVisit(e.subExpr))
        };
    }

    constexpr void checkExprUse(NodeHandle<ExprNode> e) {
        CheckExprUsage use(ast);
        use.dispatchVisit(e);
    }
    constexpr void markInitialized(NodeHandle<ExprNode> handle) {
        NodeStorage& storage = ast.node(handle);
        std::visit(
            [&]<typename T>(T& node) {
                if constexpr (std::is_same_v<T, VarExprNode>) {
                    ast.varContext.varDecl(node.varDecl).isInitialized = true;
                } else {
                    VERIFY_NOT_REACHED(ShaderGen);
                }
            },
            storage);
    }
    constexpr AssignmentStmtNode visitAssignmentStmt(auto s) {
        auto left = ast.emit(dispatchVisit(s.left));
        auto right = ast.emit(dispatchVisit(s.right));
        checkExprUse(right);
        markInitialized(left);
        return {
            {},
            left,
            right
        };
    }
};

}
