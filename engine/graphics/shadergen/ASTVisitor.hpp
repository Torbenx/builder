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
        const auto* vType = asVectorType(type);
        out << vType->scalarType->name << vType->vectorSize;
    } else if (type->isMatrix) {
        const auto* mType = asMatrixType(type);
        out << mType->scalarType->name << mType->vectorSize << "x" << mType->covectorSize;
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

constexpr bool isExpr(const Stmt* stmt) {
#define STMT_KIND(S)  \
    case StmtKind::S: \
        return std::is_base_of_v<Expr, S>;

    switch (stmt->stmtKind) {
        ENUMERATE_STMT_KINDS(STMT_KIND)
    default:
        return false;
    }
#undef STMT_KIND
}

template<typename T>
struct PseudoVector {
    T* m_begin = nullptr;
    int_t* sizePtr = nullptr;
    constexpr T* acquireEnd(int_t size = 1) {
        T* end = this->end();
        *sizePtr += size;
        return end;
    }
    constexpr void pushBack(const T& item) {
        *acquireEnd() = item;
    }
    constexpr T* begin() const { return m_begin; }
    constexpr T* end() const { return begin() + size(); }
    constexpr int_t size() const { return *sizePtr; }
};

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
    constexpr RetT visitMatrixMultiplicationExpr(const MatrixMultiplicationExpr* e, Args... args) {
        helper h([&]() {
            impl()->dispatchVisit(e->left, args...);
            impl()->dispatchVisit(e->right, args...);
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
    constexpr void visitDeclContext(const VarDeclContext* declContext, Args... args) { }
    constexpr RetT visitBlockStmt(const BlockStmt* e, Args... args) {
        helper h([&]() {
            impl()->visitDeclContext(&e->declContext, args...);
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
        helper h([&]() {
            impl()->visitErrorNode(e, args...);
        });
        return impl()->dispatchVisit(e->subStmt, args...);
    }
    constexpr RetT visitErrorExpr(const ErrorExpr* e, Args... args) {
        helper h([&]() {
            impl()->visitErrorNode(e, args...);
        });
        return impl()->dispatchVisit(e->subExpr, args...);
    }
    constexpr void visitErrorNode(const ErrorNode* e, Args... args) {
        e->notes.forEachEntry([&](const ErrorNote* n) {
            impl()->visitNote(n, args...);
        });
    }
    constexpr void visitNote(const ErrorNote* n, Args... args) { }

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

    constexpr void visitStageDecl(const StageDecl* e) {
        impl()->visitDeclContext(&e->builtins);
        impl()->visitDeclContext(&e->uniforms);
        impl()->visitDeclContext(&e->inputs);
        impl()->visitDeclContext(&e->outputs);
        impl()->dispatchVisit(e->entryPoint);
    }
};

struct PrevDeclStripper : ASTVisitor<PrevDeclStripper> {
    using Base = ASTVisitor<PrevDeclStripper>;
    constexpr void visitDeclContext(const VarDeclContext* declContext) {
        declContext->decls.forEachEntry([](VarDecl* decl) {
            decl->prevDecl = nullptr;
        });
    }
};

struct UserTypeRecorder : ASTVisitor<UserTypeRecorder> {
    using Base = ASTVisitor<UserTypeRecorder>;
    PseudoVector<const UserType*> buffer;
    constexpr UserTypeRecorder(PseudoVector<const UserType*> buffer)
        : buffer(buffer) { }
    constexpr void recordType(const Type* type) {
        if (!type->isUser)
            return;
        const auto* userType = asUserType(type);
        if (std::find(buffer.begin(), buffer.end(), userType) == buffer.end())
            buffer.pushBack(userType);
    }
    constexpr void visitExpr(const Expr* expr) {
        Base::visitExpr(expr);
        recordType(expr->type);
    }
    constexpr void visitDeclContext(const VarDeclContext* context) {
        context->decls.forEachEntry([&](VarDecl* decl) {
            recordType(decl->type);
        });
    }
};

union ASTNodeStorage {
#define AST_NODE(N) N N##_;
    ENUMERATE_AST_NODES(AST_NODE)
#undef AST_NODE

    constexpr Stmt* setStmt(const Stmt& s) {
#define STMT_KIND(S)  \
    case StmtKind::S: \
        return std::construct_at(&(S##_), (const S&)s);

        switch (s.stmtKind) {
            ENUMERATE_STMT_KINDS(STMT_KIND)
        default:
            VERIFY_NOT_REACHED(ShaderGen);
        }
#undef STMT_KIND
    }
    constexpr ASTListEntry* setListEntry(const ASTListEntry& e) {
        return std::construct_at(&ASTListEntry_, e);
    }
    constexpr VarDecl* setVarDecl(const VarDecl& decl) {
        return std::construct_at(&VarDecl_, decl);
    }
    constexpr ErrorNote* setNote(const ErrorNote& note) {
        return std::construct_at(&ErrorNote_, note);
    }
    constexpr StageDecl* setStageDecl(const StageDecl& stage) {
        return std::construct_at(&StageDecl_, stage);
    }

    constexpr ASTNodeStorage() { }
};
struct ASTStorage {
    PseudoVector<ASTNodeStorage> nodeStorage;
    PseudoVector<char> messageStorage;
    template<typename T>
    constexpr T* allocateStmt(const T& stmt) {
        return (T*)nodeStorage.acquireEnd()->setStmt(stmt);
    }
    constexpr VarDecl* allocateVarDecl(const VarDecl& decl) {
        return nodeStorage.acquireEnd()->setVarDecl(decl);
    }
    template<typename T>
    constexpr void pushBackOnList(ASTList<T>& list, T* e) {
        ASTListEntry* entry = nodeStorage.acquireEnd()->setListEntry({ e });
        list.pushBack(entry);
    }
    constexpr ErrorNote* allocateNote(const ErrorNote& note) {
        return nodeStorage.acquireEnd()->setNote(note);
    }
    constexpr StageDecl* allocateStageDecl(const StageDecl& stage) {
        return nodeStorage.acquireEnd()->setStageDecl(stage);
    }
};

template<typename Impl, typename Storage = ASTStorage>
struct ASTTransform {
    constexpr Impl* impl() { return static_cast<Impl*>(this); }

    VarDeclContext* currentDeclContext = nullptr;
    Storage storage = {};

    constexpr ASTTransform() = default;
    constexpr ASTTransform(Storage storage)
        : storage(storage) { }

    constexpr void transformStmt(Stmt* e) { }
    constexpr void transformExpr(Expr* e) {
        impl()->transformStmt(e);
    }
    constexpr void transformVarRefExpr(VarRefExpr* e) {
        VarDecl* oldDecl = e->decl;
        VarDecl* newDecl = nullptr;
        if (oldDecl->declContext == &globalPseudoVarDeclContext) {
            newDecl = oldDecl;
        } else {
            VarDeclContext* declContext = currentDeclContext;
            while (declContext && !newDecl) {
                declContext->decls.forEachEntry([&](VarDecl* decl) {
                    if (decl->prevDecl == oldDecl) {
                        VERIFY(ShaderGen, !newDecl);
                        newDecl = decl;
                    }
                });
                declContext = declContext->parent;
            }
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
    constexpr void transformMatrixMultiplicationExpr(MatrixMultiplicationExpr* e) {
        impl()->transformExpr(e);
        e->left = castStmt<Expr>(impl()->dispatchTransform(e->left));
        e->right = castStmt<Expr>(impl()->dispatchTransform(e->right));
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
            storage.pushBackOnList(*e, impl()->dispatchTransform(stmt));
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
            storage.pushBackOnList(e->arguments, castStmt<const Expr>(impl()->dispatchTransform(expr)));
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
        e->subExpr = castStmt<Expr>(impl()->dispatchTransform(e->subExpr));
    }
    constexpr void transformErrorNode(ErrorNode* e) {
        e->message = transformMessage(e->message);
        e->notes.consumeEntries([&](const ErrorNote* n) {
            storage.pushBackOnList(e->notes, impl()->createNote(*n));
        });
    }
    constexpr void transformNote(ErrorNote* e) {
        e->message = impl()->transformMessage(e->message);
    }
    constexpr std::string_view transformMessage(std::string_view in) { return in; }

    constexpr void transformStageDeclContext(StageDecl* e, VarDeclContext* parentContext = nullptr) {
        e->builtins.parent = parentContext;
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
        StageDecl* out = storage.allocateStageDecl(in);
        impl()->transformStageDecl(out);
        return out;
    }
    constexpr void initializeEmptyStageDecl(StageDecl* e, VarDeclContext* parentContext = nullptr) {
        impl()->transformStageDeclContext(e, parentContext);
    }
    constexpr StageDecl* createEmptyStageDecl() {
        StageDecl* out = storage.allocateStageDecl({});
        initializeEmptyStageDecl(out);
        return out;
    }

#define STMT_KIND(S)                         \
    constexpr Stmt* create##S(const S& in) { \
        S* out = storage.allocateStmt(in);   \
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
        VarDecl* out = storage.allocateVarDecl(*in);
        out->declContext = declContext;
        out->prevDecl = in;
        storage.pushBackOnList(declContext->decls, out);
        return out;
    }
    constexpr ErrorNote* createNote(const ErrorNote& in) {
        ErrorNote* out = storage.allocateNote(in);
        impl()->transformNote(out);
        return out;
    }
};
}
