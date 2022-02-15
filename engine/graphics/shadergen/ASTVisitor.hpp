#pragma once

#include "PersistentAST.hpp"
#include "TemplateAST.hpp"
#include "util/log.hpp"

namespace shadergen {

template<typename Impl, typename RetT = void>
struct ASTVisitor {
    const PersistentAST* ast = nullptr;
    constexpr ASTVisitor() { }
    constexpr ASTVisitor(const PersistentAST& ast)
        : ast(&ast) { }
    constexpr Impl* impl() { return static_cast<Impl*>(this); }

    constexpr RetT visitExpr(auto) {
        return void();
    }
    constexpr RetT visitVarExpr(auto e) {
        return impl()->visitExpr(e);
    }
    constexpr RetT visitElementwiseBinaryOperatorExpr(auto e) {
        dispatchVisit(e.left());
        dispatchVisit(e.right());
        return impl()->visitExpr(e);
    }
    constexpr RetT visitVectorByScalarBinaryOperatorExpr(auto e) {
        dispatchVisit(e.vector());
        dispatchVisit(e.scalar());
        return impl()->visitExpr(e);
    }
    constexpr RetT visitUnaryOperatorExpr(auto e) {
        dispatchVisit(e.subExpr());
        return impl()->visitExpr(e);
    }

    template<typename T>
    constexpr Type<T> getType(Type<T>) { return {}; }
    constexpr TypeInfo getType(TypeHandle handle) {
        return impl()->typeContext().type(handle);
    }

    template<typename T>
    constexpr VarDecl getVarDecl(VarHandle handle) {
        return impl()->varContext().varDecl(handle);
    }

    constexpr const TypeContext& typeContext() { return ast->typeContext; }
    constexpr const VarDeclContext& varContext() { return ast->varContext; }

    template<typename E>
    constexpr E wrap(E e) { return e; }

    // template dispatch
    template<typename T>
    constexpr RetT dispatchVisit(Expr<T> e) {
        return impl()->visitExpr(impl()->wrap(e));
    }
    template<typename T, fixed_string name>
    constexpr RetT dispatchVisit(VarExpr<T, name> e) {
        return impl()->visitVarExpr(impl()->wrap(e));
    }
    template<typename T, BinaryOperatorEnum op, typename Left, typename Right>
    constexpr RetT dispatchVisit(ElementwiseBinaryOperatorExpr<T, op, Left, Right> e) {
        return impl()->visitElementwiseBinaryOperatorExpr(impl()->wrap(e));
    }
    template<typename T, BinaryOperatorEnum op, typename Vector, typename Scalar, bool scalarIsLeft>
    constexpr RetT dispatchVisit(VectorByScalarBinaryOperatorExpr<T, op, Vector, Scalar, scalarIsLeft> e) {
        return impl()->visitVectorByScalarBinaryOperatorExpr(impl()->wrap(e));
    }
    template<typename T, UnaryOperatorEnum op, typename SubExpr>
    constexpr RetT dispatchVisit(UnaryOperatorExpr<T, op, SubExpr> e) {
        return impl()->visitUnaryOperatorExpr(impl()->wrap(e));
    }

    // persistent dispatch
    constexpr RetT dispatchVisit(NodeHandle<ExprNode> handle) {
        NodeStorage storage = ast->node(handle);
        return std::visit(
            [&](auto node) -> RetT {
                if constexpr (std::is_same_v<decltype(node), ExprNode>)
                    return impl()->visitExpr(node);
                else if constexpr (std::is_same_v<decltype(node), VarExprNode>)
                    return impl()->visitVarExpr(node);
                else if constexpr (std::is_same_v<decltype(node), ElementwiseBinaryOperatorExprNode>)
                    return impl()->visitElementwiseBinaryOperatorExpr(node);
                else if constexpr (std::is_same_v<decltype(node), VectorByScalarBinaryOperatorExprNode>)
                    return impl()->visitVectorByScalarBinaryOperatorExpr(node);
                else if constexpr (std::is_same_v<decltype(node), UnaryOperatorExprNode>)
                    return impl()->visitUnaryOperatorExpr(node);
                else
                    VERIFY_NOT_REACHED(ShaderGen);
            },
            storage);
    }
};

}
