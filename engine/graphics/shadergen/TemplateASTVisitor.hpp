#pragma once

#include "TemplateAST.hpp"
#include "util/log.hpp"

namespace shadergen {

template<typename Impl>
struct TemplateASTVisitor {
    constexpr Impl* impl() { return static_cast<Impl*>(this); }

    constexpr auto visitStmt(auto) {
        return void();
    }
    constexpr auto visitExpr(auto e) {
        return impl()->visitStmt(e);
    }
    constexpr auto visitVarExpr(auto e) {
        return impl()->visitExpr(e);
    }
    constexpr auto visitElementwiseBinaryOperatorExpr(auto e) {
        impl()->dispatchVisit(e.left);
        impl()->dispatchVisit(e.right);
        return impl()->visitExpr(e);
    }
    constexpr auto visitVectorByScalarBinaryOperatorExpr(auto e) {
        impl()->dispatchVisit(e.vector);
        impl()->dispatchVisit(e.scalar);
        return impl()->visitExpr(e);
    }
    constexpr auto visitUnaryOperatorExpr(auto e) {
        impl()->dispatchVisit(e.subExpr);
        return impl()->visitExpr(e);
    }
    constexpr auto visitAssignmentStmt(auto e) {
        impl()->dispatchVisit(e.left);
        impl()->dispatchVisit(e.right);
        return impl()->visitStmt(e);
    }
    constexpr auto visitLiteralExpr(auto e) {
        return impl()->visitExpr(e);
    }

    template<typename T, ValueKind valKind>
    constexpr Type<T> getType(Expr<T, valKind>) { return {}; }

    template<fixed_string name_>
    struct MockVarDecl {
        constexpr std::string_view name() { return name_.view(); }
    };
    template<typename T, fixed_string name>
    constexpr MockVarDecl<name> getVarDecl(VarExpr<T, name>) { return {}; }

    // dispatch
    template<typename T>
    constexpr auto dispatchVisit(Expr<T> e) {
        return impl()->visitExpr(e);
    }
    template<typename T, fixed_string name>
    constexpr auto dispatchVisit(VarExpr<T, name> e) {
        return impl()->visitVarExpr(e);
    }
    template<typename T, BinaryOperatorEnum op, typename Left, typename Right>
    constexpr auto dispatchVisit(ElementwiseBinaryOperatorExpr<T, op, Left, Right> e) {
        return impl()->visitElementwiseBinaryOperatorExpr(e);
    }
    template<typename T, BinaryOperatorEnum op, typename Vector, typename Scalar, bool scalarIsLeft>
    constexpr auto dispatchVisit(VectorByScalarBinaryOperatorExpr<T, op, Vector, Scalar, scalarIsLeft> e) {
        return impl()->visitVectorByScalarBinaryOperatorExpr(e);
    }
    template<typename T, UnaryOperatorEnum op, typename SubExpr>
    constexpr auto dispatchVisit(UnaryOperatorExpr<T, op, SubExpr> e) {
        return impl()->visitUnaryOperatorExpr(e);
    }
    template<typename Left, typename Right>
    constexpr auto dispatchVisit(AssignmentStmt<Left, Right> s) {
        return impl()->visitAssignmentStmt(s);
    }
    template<typename T>
    constexpr auto dispatchVisit(LiteralExpr<T> e) {
        return impl()->visitLiteralExpr(e);
    }
};

}
