#pragma once

#include "Stmt.hpp"
#include <vector>

template<>
struct fmt::formatter<shadergen::DynamicName> {
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(shadergen::DynamicName name, FormatContext& ctx) {
        return format_to(ctx.out(), "{}", name.view());
    }
};

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
template<typename Impl, typename RetT = void>
struct ASTVisitor {
    constexpr Impl* impl() { return static_cast<Impl*>(this); }

    constexpr RetT visitStmt(const Stmt&) {
        return void();
    }
    constexpr RetT visitExpr(const Expr& e) {
        return impl()->visitStmt(e);
    }
    constexpr RetT visitVarRefExpr(const VarRefExpr& e) {
        return impl()->visitExpr(e);
    }
    constexpr RetT visitElementwiseOperatorExpr(const ElementwiseOperatorExpr& e) {
        impl()->dispatchVisit(e.left);
        impl()->dispatchVisit(e.right);
        return impl()->visitExpr(e);
    }
    constexpr RetT visitVectorByScalarOperatorExpr(const VectorByScalarOperatorExpr& e) {
        impl()->dispatchVisit(e.vector);
        impl()->dispatchVisit(e.scalar);
        return impl()->visitExpr(e);
    }
    constexpr RetT visitUnaryOperatorExpr(const UnaryOperatorExpr& e) {
        impl()->dispatchVisit(e.subExpr);
        return impl()->visitExpr(e);
    }
    constexpr RetT visitAssignmentStmt(const AssignmentStmt& e) {
        impl()->dispatchVisit(e.left);
        impl()->dispatchVisit(e.right);
        return impl()->visitStmt(e);
    }
    constexpr RetT visitLiteralExpr(const LiteralExpr& e) {
        return impl()->visitExpr(e);
    }
    constexpr RetT visitVarDeclStmt(const VarDeclStmt& e) {
        return impl()->visitStmt(e);
    }
    constexpr RetT visitCompoundStmt(const CompoundStmt& e) {
        e.forEachStmt([&](const Stmt& stmt) {
            impl()->dispatchVisit(stmt);
        });
        return impl()->visitStmt(e);
    }
    constexpr RetT visitBlockStmt(const BlockStmt& e) {
        return impl()->visitCompoundStmt(e);
    }
    constexpr RetT visitMemberAccessExpr(const MemberAccessExpr& e) {
        impl()->dispatchVisit(e.base);
        return impl()->visitExpr(e);
    }

    constexpr RetT dispatchVisit(const Stmt& stmt) {
        switch (stmt.stmtKind) {
#define STMT_KIND(S)               \
    case StmtKind::S:              \
        return impl()->visit##S((const S&)stmt);

            ENUMERATE_STMT_KINDS(STMT_KIND)
#undef STMT_KIND
        default:
            VERIFY_NOT_REACHED(ShaderGen);
        }
    }
};

struct ASTDumper : ASTVisitor<ASTDumper> {
    using Base = ASTVisitor<ASTDumper>;
    int_t curContext = -1;
    int_t indent = -1;
    struct DumpContext {
        int_t parent;
        int_t indent;
        DumpContext(int_t parent, int_t indent)
            : parent(parent), indent(indent) { }
        std::string nodeType;
        std::vector<std::string> infos;

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
    };
    std::vector<DumpContext> stack;

    template<typename... Args>
    void info(std::string type, Args&&... args) {
        DumpContext& ctx = stack[curContext];
        if (ctx.nodeType.empty())
            ctx.nodeType = type;
        (ctx.infos.emplace_back(std::forward<Args>(args)), ...);
    }

    void visitStmt(const Stmt& e) {
        info("Stmt");
        Base::visitStmt(e);
    }
    void visitExpr(const Expr& e) {
        info("Expr", fmt::format("{}", *e.type));
        Base::visitExpr(e);
    }
    void visitVarRefExpr(const VarRefExpr& e) {
        info("VarRefExpr" , fmt::format("\"{}\"", e.decl.name));
        Base::visitVarRefExpr(e);
    }
    void visitElementwiseOperatorExpr(const ElementwiseOperatorExpr& e) {
        info("ElementwiseOperatorExpr", fmt::format("'{}'", toString(e.op)));
        Base::visitElementwiseOperatorExpr(e);
    }
    void visitVectorByScalarOperatorExpr(const VectorByScalarOperatorExpr& e) {
        info("VectorByScalarOperatorExpr", fmt::format("'{}'", toString(e.op)));
        Base::visitVectorByScalarOperatorExpr(e);
    }
    void visitUnaryOperatorExpr(const UnaryOperatorExpr& e) {
        info("UnaryOperatorExpr", fmt::format("'{}'", toString(e.op)));
        Base::visitUnaryOperatorExpr(e);
    }
    void visitAssignmentStmt(const AssignmentStmt& e) {
        info("AssignmentStmt");
        Base::visitAssignmentStmt(e);
    }
    void visitLiteralExpr(const LiteralExpr& e) {
        info("LiteralExpr");
        Base::visitLiteralExpr(e);
    }
    void visitVarDeclStmt(const VarDeclStmt& e) {
        info("VarDeclStmt", fmt::format("{}", *e.type), fmt::format("\"{}\"", e.name));
        Base::visitVarDeclStmt(e);
    }
    void visitCompoundStmt(const CompoundStmt& e) {
        info("CompoundStmt", fmt::format("({} children)", e.childCount));
        Base::visitCompoundStmt(e);
    }
    void visitBlockStmt(const BlockStmt& e) {
        info("BlockStmt");
        Base::visitBlockStmt(e);
    }
    void visitMemberAccessExpr(const MemberAccessExpr& e) {
        info("MemberAccessExpr", fmt::format("[{}]", e.memberIndex));
        Base::visitMemberAccessExpr(e);
    }

    void dispatchVisit(const Stmt& e) {
        indent += 1;
        stack.emplace_back(curContext, indent);
        curContext = stack.size() - 1;
        Base::dispatchVisit(e);
        curContext = stack[curContext].parent;
        indent -= 1;
    }

    void dump(const Stmt& e) {
        this->dispatchVisit(e);
        for (auto& ctx : stack)
            ctx.print();
        stack.clear();
    }
};

}
