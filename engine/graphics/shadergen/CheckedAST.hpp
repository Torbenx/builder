#pragma once

#include "ASTVisitor.hpp"

namespace shadergen {

template<typename Impl, typename RetT = void>
struct CheckedASTVisitor : ASTVisitor<Impl, RetT> {
    using ASTVisitor<Impl, RetT>::ASTVisitor;
    template<typename E>
    struct ExprWrapper : E {
        TypeHandle m_type;
        constexpr TypeHandle type() { return m_type; }
    };
    template<typename E>
    struct VarExprWrapper : ExprWrapper<E> {
        VarHandle m_varDecl;
        constexpr VarHandle varDecl() { return m_varDecl; }
    };

    constexpr Impl* impl() { return static_cast<Impl*>(this); }

    template<typename E>
    constexpr ExprWrapper<E> wrapExpr(E e) {
        TypeContext& typeContext = impl()->typeContext();
        return ExprWrapper<E> { e, typeContext.findType(e.type()) };
    }

    template<typename E>
    constexpr auto wrap(E e) requires IsExpr<E> {
        return wrapExpr(e);
    }

    template<typename E>
    constexpr auto wrap(E e) requires IsExpr<E> && E::IS_VAR_EXPR {
        VarDeclContext& declContext = impl()->varDeclContext();
        return VarExprWrapper<E> { wrapExpr(e), declContext.findVarDecl(e) };
    }
};

}
