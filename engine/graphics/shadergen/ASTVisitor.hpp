#pragma once

#include "Stmt.hpp"
#include <sstream>
#include <vector>

template<>
struct fmt::formatter<shadergen::Type> {
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const shadergen::Type& type, FormatContext& ctx) {
        if (type.isScalar) {
            return format_to(ctx.out(), "[{}]", shadergen::asScalarType(&type)->name);
        } else if (type.isVector) {
            const auto* vType = shadergen::asVectorType(&type);
            return format_to(ctx.out(), "[{} x{}]", vType->scalarType->name, vType->vectorSize);
        } else {
            return format_to(ctx.out(), "[unknown]");
        }
    }
};

namespace shadergen {

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
        e->body = impl()->dispatchTransform(e->body);
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
};

struct ASTDumper : ASTVisitor<ASTDumper> {
    using Base = ASTVisitor<ASTDumper>;
    int_t nextIndent = 0;
    int_t indent = 0;
    std::string nodeType;
    std::vector<std::string> infos;

    template<typename... Args>
    void info(std::string type, Args&&... args) {
        if (nodeType.empty())
            nodeType = type;
        (infos.emplace_back(std::forward<Args>(args)), ...);
    }

    void print() {
        std::string out(indent * 2, ' ');
        out += nodeType;
        for (auto info : infos) {
            out += ' ';
            out += info;
        }
        log("{}", out);
        nodeType = "";
        infos.clear();
    }

    void visitStmt(const Stmt* e) {
        info("Stmt");
        Base::visitStmt(e);
    }
    void visitExpr(const Expr* e) {
        info("Expr", fmt::format("{}", *e->type));
        Base::visitExpr(e);
    }
    void visitVarRefExpr(const VarRefExpr* e) {
        info("VarRefExpr", fmt::format("\"{}\"", e->decl->name));
        Base::visitVarRefExpr(e);
    }
    void visitElementwiseOperatorExpr(const ElementwiseOperatorExpr* e) {
        info("ElementwiseOperatorExpr", fmt::format("'{}'", toString(e->op)));
        Base::visitElementwiseOperatorExpr(e);
    }
    void visitVectorByScalarOperatorExpr(const VectorByScalarOperatorExpr* e) {
        info("VectorByScalarOperatorExpr", fmt::format("'{}'", toString(e->op)));
        Base::visitVectorByScalarOperatorExpr(e);
    }
    void visitUnaryOperatorExpr(const UnaryOperatorExpr* e) {
        info("UnaryOperatorExpr", fmt::format("'{}'", toString(e->op)));
        Base::visitUnaryOperatorExpr(e);
    }
    void visitAssignmentStmt(const AssignmentStmt* e) {
        info("AssignmentStmt");
        Base::visitAssignmentStmt(e);
    }
    void visitLiteralExpr(const LiteralExpr* e) {
        std::stringstream literal;
        literal << "{";
        const auto* vType = asVectorType(e->type);
        if (vType->scalarType == &BoolType) {
            for (int_t i = 0; i < vType->vectorSize; i++)
                literal << fmt::format("{:,^{}} {}", "", (int)(i > 0), e->literal.bools[i]);
        } else if (vType->scalarType == &IntType) {
            for (int_t i = 0; i < vType->vectorSize; i++)
                literal << fmt::format("{:,^{}} {}", "", (int)(i > 0), e->literal.ints[i]);
        } else if (vType->scalarType == &FloatType) {
            for (int_t i = 0; i < vType->vectorSize; i++)
                literal << fmt::format("{:,^{}} {:.2f}", "", (int)(i > 0), e->literal.floats[i]);
        }
        literal << " }";
        info("LiteralExpr", literal.str());
        Base::visitLiteralExpr(e);
    }
    void visitVarDeclStmt(const VarDeclStmt* e) {
        info("VarDeclStmt", fmt::format("{}", *e->decl->type), fmt::format("\"{}\"", e->decl->name));
        Base::visitVarDeclStmt(e);
    }
    void visitCompoundStmt(const CompoundStmt* e) {
        info("CompoundStmt", fmt::format("({} children)", e->entryCount));
        Base::visitCompoundStmt(e);
    }
    void visitBlockStmt(const BlockStmt* e) {
        info("BlockStmt", fmt::format("({} decls)", e->declContext.decls.entryCount));
        Base::visitBlockStmt(e);
    }
    void visitMemberAccessExpr(const MemberAccessExpr* e) {
        info("MemberAccessExpr", fmt::format(".{}", e->type->members[e->memberIndex].name));
        Base::visitMemberAccessExpr(e);
    }
    void visitVectorComponentExpr(const VectorComponentExpr* e) {
        if (e->componentCount == 0) {
            info("VectorComponentExpr", "'?'");
        } else {
            static constexpr char characters[4] = { 'x', 'y', 'z', 'w' };
            auto getChar = [](int8_t comp) { return comp < 4 ? characters[comp] : '?'; };
            std::stringstream ss;
            ss << "'.";
            for (int_t i = 0; i < e->componentCount; i++)
                ss << getChar(e->components[i]);
            ss << '\'';
            info("VectorComponentExpr", ss.str());
        }
        Base::visitVectorComponentExpr(e);
    }
    void visitCallExpr(const CallExpr* e) {
        info("CallExpr", fmt::format("\"{}\"", e->function->name), fmt::format("({} arguments)", e->arguments.entryCount));
        Base::visitCallExpr(e);
    }

    void dispatchVisit(const Stmt* e) {
        if (!nodeType.empty()) {
            print();
        }
        indent = nextIndent;
        nextIndent += 1;
        Base::dispatchVisit(e);
        nextIndent -= 1;
    }

    void dump(const Stmt* e) {
        this->dispatchVisit(e);
        print();
    }
};

}
