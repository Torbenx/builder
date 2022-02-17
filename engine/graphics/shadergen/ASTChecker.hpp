#pragma once

#include "TemplateASTVisitor.hpp"
#include "context.hpp"

namespace shadergen {

struct CheckExprUsage : TemplateASTVisitor<CheckExprUsage> {
    VarDeclContext& ctx;
    constexpr CheckExprUsage(VarDeclContext& ctx)
        : ctx(ctx) { }

    constexpr void visitVarExpr(auto e) {
        VERIFY(ShaderGen, ctx.varDecl(e.varDecl).isInitialized);
    }
};

struct ASTChecker : TemplateASTVisitor<ASTChecker> {
    VarDeclContext& varContext;
    constexpr ASTChecker(VarDeclContext& ctx)
        : varContext(ctx) { }
    constexpr void visitVarExpr(auto e) {
        VERIFY(ShaderGen, varContext.varDecl(e.varDecl).name() == e.name());
    }

    constexpr void checkExprUse(auto e) {
        CheckExprUsage use(varContext);
        use.dispatchVisit(e);
    }
    template<typename T, fixed_string name>
    constexpr void markInitialized(VarExpr<T, name> e) {
        varContext.varDecl(e.varDecl).isInitialized = true;
    }
    constexpr void visitAssignmentStmt(auto s) {
        checkExprUse(s.right);
        markInitialized(s.left);
    }
};

};
