#pragma once

#include "util/log.hpp"
#include "util/reflection.hpp"
#include "util/types.hpp"
#include "util/uninitialized.hpp"

#include "PersistentASTBuilder.hpp"

namespace shadergen {

struct ASTDumper : ASTVisitor<ASTDumper> {
    using ASTVisitor<ASTDumper>::ASTVisitor;
    int_t indent = 0;
    template<typename... Args>
    void dump(fmt::format_string<Args...> str, Args&&... args) {
        log("{:{}}{}", "", indent, fmt::format(str, std::forward<Args>(args)...));
    }

    void visitExpr(auto e) {
        dump("{} Expr", getType(e.type()));
    }
    void visitVarExpr(auto e) {
        dump("{} VarExpr \"{}\"", getType(e.type()), getVarDecl(e.varDecl()).name());
    }
    void visitElementwiseBinaryOperatorExpr(auto e) {
        dump("{} ElementwiseBinaryOperatorExpr '{}'", getType(e.type()), toString(e.op()));
        visit(e.left());
        visit(e.right());
    }
    void visitVectorByScalarBinaryOperatorExpr(auto e) {
        dump("{} VectorByScalarBinaryOperatorExpr '{}'", getType(e.type()), toString(e.op()));
        visit(e.vector());
        visit(e.scalar());
    }
    void visitUnaryOperatorExpr(auto e) {
        dump("{} UnaryOperatorExpr '{}'", getType(e.type()), toString(e.op()));
        visit(e.subExpr());
    }

    void visit(auto e) {
        indent += 2;
        dispatchVisit(e);
        indent -= 2;
    }
};

template<typename Stmt>
void dump(Stmt stmt) {
    ASTDumper dumper;
    dumper.dispatchVisit(stmt);
}
}
