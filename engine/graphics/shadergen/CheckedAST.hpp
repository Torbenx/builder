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

    constexpr const TypeInfo& getType(TypeHandle handle) {
        return impl()->typeContext().type(handle);
    }

    constexpr const VarDecl& getVarDecl(VarHandle handle) {
        return impl()->varContext().varDecl(handle);
    }
    template<typename E>
    constexpr ExprWrapper<E> wrapExpr(E e) {
        TypeContext& typeContext = impl()->typeContext();
        auto oldType = ASTVisitor<Impl, RetT>::getType(e.type());
        auto newType = typeContext.findType(oldType);
        return ExprWrapper<E> { e, newType };
    }
    template<typename E>
    constexpr VarExprWrapper<E> wrapVarExpr(E e) {
        VarDeclContext& declContext = impl()->varDeclContext();
        auto oldDecl = ASTVisitor<Impl, RetT>::getVarDecl(e.varDecl());
        auto newDecl = declContext.findVarDecl(oldDecl.name());
        return VarExprWrapper<E> { wrapExpr(e), newDecl };
    }

    template<typename E>
    constexpr auto wrap(const E& e) requires std::derived_from<E, ExprNode> {
        return wrapExpr(e);
    }

    template<typename E>
    constexpr auto wrap(const E& e) requires std::derived_from<E, ExprNode> && std::derived_from<E, VarExprNode> {
        return wrapVarExpr(e);
    }

    template<typename E>
    constexpr auto wrap(E e) requires IsExpr<E> {
        return wrapExpr(e);
    }

    template<typename E>
    constexpr auto wrap(E e) requires IsExpr<E> && E::IS_VAR_EXPR {
        return wrapVarExpr(e);
    }
};

}
