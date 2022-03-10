#pragma once

#include "functions.hpp"
#include "operators.hpp"
#include <sstream>
#include <vector>

namespace shadergen {

template<typename Stream>
constexpr void formatType(const Type* type, Stream& out) {
    if (type->isScalar) {
        out << asScalarType(type)->name;
    } else if (type->isVector) {
        const auto* vType = shadergen::asVectorType(type);
        out << vType->scalarType->name << vType->vectorSize;
    } else if (type->isUser) {
        out << asUserType(type)->name;
    } else if (type->isError) {
        out << "error";
    } else {
        out << "unknown";
    }
}

}

template<>
struct fmt::formatter<shadergen::Type> {
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const shadergen::Type& type, FormatContext& ctx) {
        std::stringstream ss;
        shadergen::formatType(&type, ss);
        return format_to(ctx.out(), "{}", ss.str());
    }
};

namespace shadergen {

static constexpr bool isExpr(const Stmt* stmt) {
#define STMT_KIND(S)  \
    case StmtKind::S: \
        return std::derived_from<S, Expr>;

    switch (stmt->stmtKind) {
        ENUMERATE_STMT_KINDS(STMT_KIND)
    default:
        return false;
    }
#undef STMT_KIND
}

// AST visitor
template<typename Impl, typename RetT = void, typename... Args>
struct ASTVisitor {
    constexpr Impl* impl() { return static_cast<Impl*>(this); }

    template<typename Callback>
    struct helper {
        Callback callback;
        constexpr helper(Callback&& callback)
            : callback(std::forward<Callback>(callback)) { }
        constexpr ~helper() { callback(); }
    };
    template<typename Callback>
    helper(Callback&&) -> helper<Callback>;

    constexpr RetT visitStmt(const Stmt*, Args... args) {
        return void();
    }
    constexpr RetT visitExpr(const Expr* e, Args... args) {
        return impl()->visitStmt(e, args...);
    }
    constexpr RetT visitVarRefExpr(const VarRefExpr* e, Args... args) {
        return impl()->visitExpr(e, args...);
    }
    constexpr RetT visitElementwiseOperatorExpr(const ElementwiseOperatorExpr* e, Args... args) {
        helper h([&]() {
            impl()->dispatchVisit(e->left, args...);
            impl()->dispatchVisit(e->right, args...);
        });
        return impl()->visitExpr(e, args...);
    }
    constexpr RetT visitVectorByScalarOperatorExpr(const VectorByScalarOperatorExpr* e, Args... args) {
        helper h([&]() {
            impl()->dispatchVisit(e->vector, args...);
            impl()->dispatchVisit(e->scalar, args...);
        });
        return impl()->visitExpr(e, args...);
    }
    constexpr RetT visitUnaryOperatorExpr(const UnaryOperatorExpr* e, Args... args) {
        helper h([&]() {
            impl()->dispatchVisit(e->subExpr, args...);
        });
        return impl()->visitExpr(e, args...);
    }
    constexpr RetT visitAssignmentStmt(const AssignmentStmt* e, Args... args) {
        helper h([&]() {
            impl()->dispatchVisit(e->left, args...);
            impl()->dispatchVisit(e->right, args...);
        });
        return impl()->visitStmt(e, args...);
    }
    constexpr RetT visitLiteralExpr(const LiteralExpr* e, Args... args) {
        return impl()->visitExpr(e, args...);
    }
    constexpr RetT visitCompoundStmt(const CompoundStmt* e, Args... args) {
        helper h([&]() {
            e->forEachEntry([&](const Stmt* stmt) {
                impl()->dispatchVisit(stmt, args...);
            });
        });
        return impl()->visitStmt(e, args...);
    }
    constexpr RetT visitBlockStmt(const BlockStmt* e, Args... args) {
        helper h([&]() {
            impl()->dispatchVisit(e->body, args...);
        });
        return impl()->visitStmt(e, args...);
    }
    constexpr RetT visitMemberAccessExpr(const MemberAccessExpr* e, Args... args) {
        helper h([&]() {
            impl()->dispatchVisit(e->base, args...);
        });
        return impl()->visitExpr(e, args...);
    }
    constexpr RetT visitVectorComponentExpr(const VectorComponentExpr* e, Args... args) {
        helper h([&]() {
            impl()->dispatchVisit(e->base, args...);
        });
        return impl()->visitExpr(e, args...);
    }
    constexpr RetT visitCallExpr(const CallExpr* e, Args... args) {
        helper h([&]() {
            e->arguments.forEachEntry([&](const Expr* expr) {
                impl()->dispatchVisit(expr, args...);
            });
        });
        return impl()->visitExpr(e, args...);
    }
    constexpr RetT visitCompositionExpr(const CompositionExpr* e, Args... args) {
        helper h([&]() {
            for (int_t i = 0; i < e->componentCount; i++)
                impl()->dispatchVisit(e->components[i], args...);
        });
        return impl()->visitExpr(e, args...);
    }
    constexpr RetT visitIfStmt(const IfStmt* e, Args... args) {
        helper h([&]() {
            impl()->dispatchVisit(e->condition, args...);
            impl()->dispatchVisit(e->whenTrue, args...);
            if (e->whenFalse)
                impl()->dispatchVisit(e->whenFalse, args...);
        });
        return impl()->visitStmt(e, args...);
    }
    constexpr RetT visitUpdateStmt(const UpdateStmt* e, Args... args) {
        helper h([&]() {
            impl()->dispatchVisit(e->left, args...);
            impl()->dispatchVisit(e->right, args...);
        });
        return impl()->visitStmt(e, args...);
    }

    constexpr RetT visitParenExpr(const ParenExpr* e, Args... args) {
        return impl()->dispatchVisit(e->subExpr, args...);
    }
    constexpr RetT visitErrorStmt(const ErrorStmt* e, Args... args) {
        return impl()->dispatchVisit(e->subStmt, args...);
    }
    constexpr RetT visitErrorExpr(const ErrorExpr* e, Args... args) {
        return impl()->dispatchVisit(e->subExpr, args...);
    }

    constexpr RetT dispatchVisit(const Stmt* stmt, Args... args) {
        switch (stmt->stmtKind) {
#define STMT_KIND(S)  \
    case StmtKind::S: \
        return impl()->visit##S((const S*)stmt, args...);

            ENUMERATE_STMT_KINDS(STMT_KIND)
#undef STMT_KIND
        default:
            VERIFY_NOT_REACHED(ShaderGen);
        }
    }
};

struct PrevDeclStripper : ASTVisitor<PrevDeclStripper> {
    using Base = ASTVisitor<PrevDeclStripper>;
    constexpr void stripDeclContext(const VarDeclContext* declContext) {
        declContext->decls.forEachEntry([](VarDecl* decl) {
            decl->prevDecl = nullptr;
        });
    }
    constexpr void visitBlockStmt(const BlockStmt* block) {
        Base::visitBlockStmt(block);
        stripDeclContext(&block->declContext);
    }
    constexpr void stripStage(const StageDecl* stageDecl) {
        stripDeclContext(&stageDecl->builtins);
        stripDeclContext(&stageDecl->uniforms);
        stripDeclContext(&stageDecl->inputs);
        stripDeclContext(&stageDecl->outputs);
        dispatchVisit(stageDecl->entryPoint);
    }
};

template<typename Impl, typename Storage>
struct ASTTransform {
    constexpr Impl* impl() { return static_cast<Impl*>(this); }

    VarDeclContext* currentDeclContext = nullptr;
    Storage* storage = nullptr;

    constexpr ASTTransform() = default;
    constexpr ASTTransform(Storage* storage)
        : storage(storage) { }

    constexpr void transformStmt(Stmt* e) { }
    constexpr void transformExpr(Expr* e) {
        impl()->transformStmt(e);
    }
    constexpr void transformVarRefExpr(VarRefExpr* e) {
        VarDeclContext* declContext = currentDeclContext;
        VarDecl* oldDecl = e->decl;
        VarDecl* newDecl = nullptr;
        while (declContext && !newDecl) {
            declContext->decls.forEachEntry([&](VarDecl* decl) {
                if (decl->prevDecl == oldDecl) {
                    VERIFY(ShaderGen, !newDecl);
                    newDecl = decl;
                }
            });
            declContext = declContext->parent;
        }
        VERIFY(ShaderGen, newDecl);
        if (!newDecl)
            VDUMP(oldDecl->name);
        e->decl = newDecl;
    }
    constexpr void transformElementwiseOperatorExpr(ElementwiseOperatorExpr* e) {
        impl()->transformExpr(e);
        e->left = castStmt<Expr>(impl()->dispatchTransform(e->left));
        e->right = castStmt<Expr>(impl()->dispatchTransform(e->right));
    }
    constexpr void transformVectorByScalarOperatorExpr(VectorByScalarOperatorExpr* e) {
        impl()->transformExpr(e);
        e->vector = castStmt<Expr>(impl()->dispatchTransform(e->vector));
        e->scalar = castStmt<Expr>(impl()->dispatchTransform(e->scalar));
    }
    constexpr void transformUnaryOperatorExpr(UnaryOperatorExpr* e) {
        impl()->transformExpr(e);
        e->subExpr = castStmt<Expr>(impl()->dispatchTransform(e->subExpr));
    }
    constexpr void transformAssignmentStmt(AssignmentStmt* e) {
        impl()->transformStmt(e);
        e->left = castStmt<Expr>(impl()->dispatchTransform(e->left));
        e->right = castStmt<Expr>(impl()->dispatchTransform(e->right));
    }
    constexpr void transformLiteralExpr(LiteralExpr* e) {
        impl()->transformExpr(e);
    }
    constexpr void transformCompoundStmt(CompoundStmt* e) {
        impl()->transformStmt(e);
        e->consumeEntries([&](const Stmt* stmt) {
            storage->pushBackOnList(*e, impl()->dispatchTransform(stmt));
        });
    }
    constexpr void transformDeclContext(VarDeclContext* declContext) {
        declContext->decls.consumeEntries([&](const VarDecl* oldDecl) {
            VarDecl* newDecl = impl()->createVarDecl(oldDecl, declContext);
        });
    }
    constexpr void transformBlockStmt(BlockStmt* e) {
        impl()->transformStmt(e);
        impl()->transformDeclContext(&e->declContext);
        e->declContext.parent = currentDeclContext;
        currentDeclContext = &e->declContext;
        e->body = castStmt<CompoundStmt>(impl()->dispatchTransform(e->body));
        currentDeclContext = e->declContext.parent;
    }
    constexpr void transformMemberAccessExpr(MemberAccessExpr* e) {
        impl()->transformExpr(e);
        e->base = castStmt<Expr>(impl()->dispatchTransform(e->base));
    }
    constexpr void transformVectorComponentExpr(VectorComponentExpr* e) {
        impl()->transformExpr(e);
        e->base = castStmt<Expr>(impl()->dispatchTransform(e->base));
    }
    constexpr void transformCallExpr(CallExpr* e) {
        impl()->transformExpr(e);
        e->arguments.consumeEntries([&](const Expr* expr) {
            storage->pushBackOnList(e->arguments, castStmt<const Expr>(impl()->dispatchTransform(expr)));
        });
    }
    constexpr void transformCompositionExpr(CompositionExpr* e) {
        impl()->transformExpr(e);
        for (int_t i = 0; i < e->componentCount; i++)
            e->components[i] = castStmt<Expr>(impl()->dispatchTransform(e->components[i]));
    }
    constexpr void transformIfStmt(IfStmt* e) {
        impl()->transformStmt(e);
        e->condition = castStmt<Expr>(impl()->dispatchTransform(e->condition));
        e->whenTrue = impl()->dispatchTransform(e->whenTrue);
        if (e->whenFalse)
            e->whenFalse = impl()->dispatchTransform(e->whenFalse);
    }
    constexpr void transformUpdateStmt(UpdateStmt* e) {
        impl()->transformStmt(e);
        e->left = castStmt<Expr>(impl()->dispatchTransform(e->left));
        e->right = castStmt<Expr>(impl()->dispatchTransform(e->right));
    }
    constexpr void transformParenExpr(ParenExpr* e) {
        impl()->transformExpr(e);
        e->subExpr = castStmt<Expr>(impl()->dispatchTransform(e->subExpr));
    }
    constexpr void transformErrorStmt(ErrorStmt* e) {
        impl()->transformStmt(e);
        e->subStmt = impl()->dispatchTransform(e->subStmt);
    }
    constexpr void transformErrorExpr(ErrorExpr* e) {
        impl()->transformExpr(e);
        e->notes.consumeEntries([&](const ErrorNote* n) {
            storage->pushBackOnList(e->notes, impl()->createNote(*n));
        });
        e->subExpr = castStmt<Expr>(impl()->dispatchTransform(e->subExpr));
    }
    constexpr void transformNote(ErrorNote* e) { }

    constexpr void transformStageDeclContext(StageDecl* e) {
        VERIFY(ShaderGen, e->builtins.parent == nullptr);
        e->uniforms.parent = &e->builtins;
        e->inputs.parent = &e->uniforms;
        e->outputs.parent = &e->inputs;
        impl()->transformDeclContext(&e->builtins);
        impl()->transformDeclContext(&e->uniforms);
        impl()->transformDeclContext(&e->inputs);
        impl()->transformDeclContext(&e->outputs);
    }
    constexpr void transformStageDecl(StageDecl* e) {
        impl()->transformStageDeclContext(e);
        currentDeclContext = &e->outputs;
        e->entryPoint = impl()->dispatchTransform(e->entryPoint);
    }
    constexpr StageDecl* createStageDecl(const StageDecl& in) {
        StageDecl* out = storage->allocateStageDecl(in);
        impl()->transformStageDecl(out);
        return out;
    }
    constexpr StageDecl* createEmptyStageDecl() {
        StageDecl* out = storage->allocateStageDecl({});
        impl()->transformStageDeclContext(out);
        return out;
    }

#define STMT_KIND(S)                         \
    constexpr Stmt* create##S(const S& in) { \
        S* out = storage->allocateStmt(in);  \
        impl()->transform##S(out);           \
        return out;                          \
    }
    ENUMERATE_STMT_KINDS(STMT_KIND)
#undef STMT_KIND

    constexpr Stmt* dispatchTransform(const Stmt* stmt) {
#define STMT_KIND(S)  \
    case StmtKind::S: \
        return impl()->create##S((const S&)*stmt);

        switch (stmt->stmtKind) {
            ENUMERATE_STMT_KINDS(STMT_KIND)
        default:
            VERIFY_NOT_REACHED(ShaderGen);
        }

#undef STMT_KIND
    }
    constexpr VarDecl* createVarDecl(const VarDecl* in, VarDeclContext* declContext) {
        VarDecl* out = storage->allocateVarDecl(*in);
        out->declContext = declContext;
        out->prevDecl = in;
        storage->pushBackOnList(declContext->decls, out);
        return out;
    }
    constexpr ErrorNote* createNote(const ErrorNote& in) {
        ErrorNote* out = storage->allocateNote(in);
        impl()->transformNote(out);
        return out;
    }
};

template<typename Storage>
struct BasicASTTransform : ASTTransform<BasicASTTransform<Storage>, Storage> {
    using Base = ASTTransform<BasicASTTransform<Storage>, Storage>;
    using ASTTransform<BasicASTTransform<Storage>, Storage>::ASTTransform;
};
}
