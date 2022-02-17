#pragma once

#include "TemplateASTVisitor.hpp"
#include "context.hpp"

namespace shadergen {

struct Checker {
    struct VarInfo {
        DynamicName name;
        bool isInitialized = false;
    };
    StaticVector<VarInfo, 100> variables;

    struct CheckExprUsage : TemplateASTVisitor<CheckExprUsage> {
        Checker& ctx;
        constexpr CheckExprUsage(Checker& ctx)
            : ctx(ctx) { }

        constexpr void visitVarExpr(auto e) {
            VERIFY(ShaderGen, ctx.variables[e.varHandle.index].isInitialized);
        }
    };

    struct ASTChecker : TemplateASTVisitor<ASTChecker> {
        Checker& ctx;
        StmtConsumerBase& scb;
        constexpr ASTChecker(Checker& ctx, StmtConsumerBase& scb)
            : ctx(ctx), scb(scb) { }

        constexpr void visitVarExpr(auto e) {
            VERIFY(ShaderGen, ctx.variables[e.varHandle.index].name.view() == e.name());
            VERIFY(ShaderGen, isSubBlock(e.varHandle.block, scb.currentBlockIndex));
        }

        constexpr void checkExprUse(auto e) {
            CheckExprUsage use { ctx };
            use.dispatchVisit(e);
        }
        template<typename T, fixed_string name>
        constexpr void markInitialized(VarExpr<T, name> e) {
            ctx.variables[e.varHandle.index].isInitialized = true;
        }
        constexpr void visitAssignmentStmt(auto s) {
            dispatchVisit(s.left);
            dispatchVisit(s.right);
            checkExprUse(s.right);
            markInitialized(s.left);
        }
    };

    template<typename T, fixed_string name>
    constexpr void declareVariable(StmtConsumerBase& scb, VarHandle handle) {
        EXPECT(ShaderGen, handle.index, variables.size());
        variables.pushBack({ name });
    }

    template<typename Stmt>
    constexpr void consumeStmt(StmtConsumerBase& scb, Stmt stmt) {
        ASTChecker checker { *this, scb };
        checker.dispatchVisit(stmt);
    }

    constexpr void beginBlock(StmtConsumerBase&) { }
    constexpr void endBlock(StmtConsumerBase&) { }
};

};
