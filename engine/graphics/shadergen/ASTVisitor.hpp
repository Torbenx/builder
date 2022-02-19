#pragma once

#include "Stmt.hpp"
#include <sstream>
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
template<typename Impl, typename RetT = void, typename... Args>
struct ASTVisitor {
    constexpr Impl* impl() { return static_cast<Impl*>(this); }

    constexpr RetT visitStmt(const Stmt&, Args... args) {
        return void();
    }
    constexpr RetT visitExpr(const Expr& e, Args... args) {
        return impl()->visitStmt(e, args...);
    }
    constexpr RetT visitVarRefExpr(const VarRefExpr& e, Args... args) {
        return impl()->visitExpr(e, args...);
    }
    constexpr RetT visitElementwiseOperatorExpr(const ElementwiseOperatorExpr& e, Args... args) {
        impl()->dispatchVisit(e.left, args...);
        impl()->dispatchVisit(e.right, args...);
        return impl()->visitExpr(e, args...);
    }
    constexpr RetT visitVectorByScalarOperatorExpr(const VectorByScalarOperatorExpr& e, Args... args) {
        impl()->dispatchVisit(e.vector, args...);
        impl()->dispatchVisit(e.scalar, args...);
        return impl()->visitExpr(e, args...);
    }
    constexpr RetT visitUnaryOperatorExpr(const UnaryOperatorExpr& e, Args... args) {
        impl()->dispatchVisit(e.subExpr, args...);
        return impl()->visitExpr(e, args...);
    }
    constexpr RetT visitAssignmentStmt(const AssignmentStmt& e, Args... args) {
        impl()->dispatchVisit(e.left, args...);
        impl()->dispatchVisit(e.right, args...);
        return impl()->visitStmt(e, args...);
    }
    constexpr RetT visitLiteralExpr(const LiteralExpr& e, Args... args) {
        return impl()->visitExpr(e, args...);
    }
    constexpr RetT visitVarDeclStmt(const VarDeclStmt& e, Args... args) {
        return impl()->visitStmt(e, args...);
    }
    constexpr RetT visitCompoundStmt(const CompoundStmt& e, Args... args) {
        e.forEachStmt([&](const Stmt& stmt) {
            impl()->dispatchVisit(stmt, args...);
        });
        return impl()->visitStmt(e, args...);
    }
    constexpr RetT visitBlockStmt(const BlockStmt& e, Args... args) {
        return impl()->visitCompoundStmt(e, args...);
    }
    constexpr RetT visitMemberAccessExpr(const MemberAccessExpr& e, Args... args) {
        impl()->dispatchVisit(e.base, args...);
        return impl()->visitExpr(e, args...);
    }
    constexpr RetT visitVectorComponentExpr(const VectorComponentExpr& e, Args... args) {
        impl()->dispatchVisit(e.base, args...);
        return impl()->visitExpr(e, args...);
    }

    constexpr RetT dispatchVisit(const Stmt& stmt, Args... args) {
        switch (stmt.stmtKind) {
#define STMT_KIND(S)  \
    case StmtKind::S: \
        return impl()->visit##S((const S&)stmt, args...);

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
        info("VarRefExpr", fmt::format("\"{}\"", e.decl.name));
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
        info("CompoundStmt", fmt::format("({} children)", e.stmtCount));
        Base::visitCompoundStmt(e);
    }
    void visitBlockStmt(const BlockStmt& e) {
        info("BlockStmt", fmt::format("({} decls)", e.declContext.decls.stmtCount));
        Base::visitBlockStmt(e);
    }
    void visitMemberAccessExpr(const MemberAccessExpr& e) {
        info("MemberAccessExpr", fmt::format(".{}", e.type->members[e.memberIndex].name));
        Base::visitMemberAccessExpr(e);
    }
    void visitVectorComponentExpr(const VectorComponentExpr& e) {
        if (e.componentCount == 0) {
            info("VectorComponentExpr", "'?'");
        } else {
            static constexpr char characters[4] = { 'x', 'y', 'z', 'w' };
            auto getChar = [](int8_t comp) { return comp < 4 ? characters[comp] : '?'; };
            std::stringstream ss;
            ss << "'.";
            for (int_t i = 0; i < e.componentCount; i++)
                ss << getChar(e.components[i]);
            ss << '\'';
            info("VectorComponentExpr", ss.str());
        }
        Base::visitVectorComponentExpr(e);
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
