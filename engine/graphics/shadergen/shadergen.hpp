#pragma once

#include "util/log.hpp"
#include "util/reflection.hpp"
#include "util/types.hpp"
#include "util/uninitialized.hpp"

#include "PersistentASTBuilder.hpp"

namespace shadergen {

template<template<typename> typename Visitor>
struct ASTDumper : Visitor<ASTDumper<Visitor>> {
    using Visitor<ASTDumper>::Visitor;
    int_t indent = 0;
    template<typename... Args>
    void dump(fmt::format_string<Args...> str, Args&&... args) {
        log("{:{}}{}", "", indent, fmt::format(str, std::forward<Args>(args)...));
    }

    void visitExpr(auto e) {
        dump("{} Expr", this->getType(e));
    }
    void visitVarExpr(auto e) {
        dump("{} VarExpr \"{}\"", this->getType(e), this->getVarDecl(e).name());
    }
    void visitElementwiseBinaryOperatorExpr(auto e) {
        dump("{} ElementwiseBinaryOperatorExpr '{}'", this->getType(e), toString(e.op()));
        visit(e.left);
        visit(e.right);
    }
    void visitVectorByScalarBinaryOperatorExpr(auto e) {
        dump("{} VectorByScalarBinaryOperatorExpr '{}'", this->getType(e), toString(e.op()));
        visit(e.vector);
        visit(e.scalar);
    }
    void visitUnaryOperatorExpr(auto e) {
        dump("{} UnaryOperatorExpr '{}'", this->getType(e), toString(e.op()));
        visit(e.subExpr);
    }
    void visitAssignmentStmt(auto e) {
        dump("AssignmentStmt");
        visit(e.left);
        visit(e.right);
    }

    void visit(auto e) {
        indent += 2;
        this->dispatchVisit(e);
        indent -= 2;
    }
};

template<typename Impl>
using PersistentASTVisitorHelper = PersistentASTVisitor<Impl>;
using TemplateASTDumper = ASTDumper<TemplateASTVisitor>;
using PersistentASTDumper = ASTDumper<PersistentASTVisitorHelper>;

}
