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

    constexpr RetT visitExpr(const auto&) {
        return void();
    }
    constexpr RetT visitVarExpr(const auto& e) {
        return impl()->visitExpr(e);
    }
    constexpr RetT visitElementwiseBinaryOperatorExpr(const auto& e) {
        dispatchVisit(e.left());
        dispatchVisit(e.right());
        return impl()->visitExpr(e);
    }
    constexpr RetT visitVectorByScalarBinaryOperatorExpr(const auto& e) {
        dispatchVisit(e.vector());
        dispatchVisit(e.scalar());
        return impl()->visitExpr(e);
    }
    constexpr RetT visitUnaryOperatorExpr(const auto& e) {
        dispatchVisit(e.subExpr());
        return impl()->visitExpr(e);
    }

    template<typename T>
    constexpr Type<T> getType(Type<T>) { return {}; }
    template<fixed_string name>
    constexpr auto getVarDecl(TemplateVarDecl<name> decl) {
        return decl;
    }

    constexpr const TypeInfo& getType(TypeHandle handle) {
        return ast->typeContext.type(handle);
    }
    constexpr const VarDecl& getVarDecl(VarHandle handle) {
        return ast->varContext.varDecl(handle);
    }

    template<typename E>
    constexpr const E& wrap(const E& e) { return e; }

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
        const NodeStorage& storage = ast->node(handle);
        return std::visit(
            [&]<typename T>(const T& node) -> RetT {
                const auto& wrapped = impl()->wrap(node);
                if constexpr (std::is_same_v<T, ExprNode>)
                    return impl()->visitExpr(wrapped);
                else if constexpr (std::is_same_v<T, VarExprNode>)
                    return impl()->visitVarExpr(wrapped);
                else if constexpr (std::is_same_v<T, ElementwiseBinaryOperatorExprNode>)
                    return impl()->visitElementwiseBinaryOperatorExpr(wrapped);
                else if constexpr (std::is_same_v<T, VectorByScalarBinaryOperatorExprNode>)
                    return impl()->visitVectorByScalarBinaryOperatorExpr(wrapped);
                else if constexpr (std::is_same_v<T, UnaryOperatorExprNode>)
                    return impl()->visitUnaryOperatorExpr(wrapped);
                else
                    VERIFY_NOT_REACHED(ShaderGen);
            },
            storage);
    }
};

}
