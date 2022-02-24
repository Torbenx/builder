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
    constexpr RetT visitVarDeclStmt(const VarDeclStmt* e, Args... args) {
        return impl()->visitStmt(e, args...);
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

template<typename Impl, typename Storage>
struct ASTTransform {
    constexpr Impl* impl() { return static_cast<Impl*>(this); }

    BlockStmt* currentBlock = nullptr;
    Storage* storage = nullptr;

    constexpr ASTTransform() = default;
    constexpr ASTTransform(Storage* storage)
        : storage(storage) { }

    constexpr void transformStmt(Stmt* e) { }
    constexpr void transformExpr(Expr* e) {
        impl()->transformStmt(e);
    }
    constexpr void transformVarRefExpr(VarRefExpr* e) {
        BlockStmt* block = currentBlock;
        VarDecl* oldDecl = e->decl;
        VarDecl* newDecl = nullptr;
        while (block && !newDecl) {
            block->declContext.decls.forEachEntry([&](VarDecl* decl) {
                if (decl->name == oldDecl->name) {
                    VERIFY(ShaderGen, !newDecl);
                    newDecl = decl;
                }
            });
            block = block->parent;
        }
        VERIFY(ShaderGen, newDecl);
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
    constexpr void transformVarDecl(VarDecl* decl) {
        decl->declContext = &currentBlock->declContext;
        storage->pushBackOnList(currentBlock->declContext.decls, decl);
    }
    constexpr void transformVarDeclStmt(VarDeclStmt* e) {
        impl()->transformStmt(e);
        e->decl = impl()->dispatchTransformVarDecl(e->decl);
    }
    constexpr void transformCompoundStmt(CompoundStmt* e) {
        impl()->transformStmt(e);
        e->consumeEntries([&](const Stmt* stmt) {
            storage->pushBackOnList(*e, impl()->dispatchTransform(stmt));
        });
    }
    constexpr void transformBlockStmt(BlockStmt* e) {
        impl()->transformStmt(e);
        e->parent = currentBlock;
        e->declContext.decls.clear();
        currentBlock = e;
        e->body = castStmt<CompoundStmt>(impl()->dispatchTransform(e->body));
        currentBlock = e->parent;
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
    constexpr void transformErrorStmt(ErrorStmt* e) {
        impl()->transformStmt(e);
        e->subStmt = impl()->dispatchTransform(e->subStmt);
    }
    constexpr void transformErrorExpr(ErrorExpr* e) {
        impl()->transformExpr(e);
        e->notes.consumeEntries([&](const ErrorNote* n) {
            storage->pushBackOnList(e->notes, impl()->dispatchTransformNote(n));
        });
        e->subExpr = castStmt<Expr>(impl()->dispatchTransform(e->subExpr));
    }
    constexpr void transformNote(ErrorNote* e) { }

    constexpr Stmt* dispatchTransform(const Stmt* stmt) {
        switch (stmt->stmtKind) {
#define STMT_KIND(S)                                     \
    case StmtKind::S: {                                  \
        S* ret = storage->allocateStmt(*(const S*)stmt); \
        impl()->transform##S(ret);                       \
        return ret;                                      \
    }

            ENUMERATE_STMT_KINDS(STMT_KIND)
#undef STMT_KIND
        default:
            VERIFY_NOT_REACHED(ShaderGen);
        }
    }
    constexpr VarDecl* dispatchTransformVarDecl(const VarDecl* decl) {
        VarDecl* ret = storage->allocateVarDecl(*decl);
        impl()->transformVarDecl(ret);
        return ret;
    }
    constexpr ErrorNote* dispatchTransformNote(const ErrorNote* note) {
        ErrorNote* ret = storage->allocateNote(*note);
        impl()->transformNote(ret);
        return ret;
    }
};

}
