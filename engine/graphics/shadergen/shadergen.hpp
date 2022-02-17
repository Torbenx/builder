#pragma once

#include "util/log.hpp"
#include "util/reflection.hpp"
#include "util/types.hpp"
#include "util/uninitialized.hpp"
#include <bit>
#include <limits>
#include <string>
#include <tuple>
#include <vector>

#include "ASTChecker.hpp"
#include "PersistentASTVisitor.hpp"

namespace shadergen {

template<template<typename> typename Visitor>
struct ASTDumper : Visitor<ASTDumper<Visitor>> {
    using Base = Visitor<ASTDumper<Visitor>>;
    using Visitor<ASTDumper>::Visitor;
    int_t curContext = -1;
    int_t indent = -2;
    struct DumpContext {
        int_t parent;
        int_t indent;
        DumpContext(int_t parent, int_t indent)
            : parent(parent), indent(indent) { }
        std::string nodeType;
        std::vector<std::string> infos;

        void print() {
            std::string out = "";
            for (auto info : infos) {
                out += ' ';
                out += info;
            }
            log("{:{}}{}{}", "", indent, nodeType, out);
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

    void visitExpr(const auto& e) {
        info("Expr", fmt::format("{}", this->getType(e)));
        Base::visitExpr(e);
    }
    void visitVarExpr(const auto& e) {
        info("VarExpr" , fmt::format("\"{}\"", this->getVarDecl(e).name()));
        Base::visitVarExpr(e);
    }
    void visitElementwiseBinaryOperatorExpr(const auto& e) {
        info("ElementwiseBinaryOperatorExpr", fmt::format("'{}'", toString(e.op())));
        Base::visitElementwiseBinaryOperatorExpr(e);
    }
    void visitVectorByScalarBinaryOperatorExpr(const auto& e) {
        info("VectorByScalarBinaryOperatorExpr", fmt::format("'{}'", toString(e.op())));
        Base::visitVectorByScalarBinaryOperatorExpr(e);
    }
    void visitUnaryOperatorExpr(const auto& e) {
        info("UnaryOperatorExpr", fmt::format("'{}'", toString(e.op())));
        Base::visitUnaryOperatorExpr(e);
    }
    void visitAssignmentStmt(const auto& e) {
        info("AssignmentStmt");
        Base::visitAssignmentStmt(e);
    }
    void visitLiteralExpr(const auto& e) {
        info("LiteralExpr");
        Base::visitLiteralExpr(e);
    }
    void visitVarDeclStmt(const auto& e) {
        info("VarDeclStmt", fmt::format("\"{}\"", e.name()));
        Base::visitVarDeclStmt(e);
    }
    void visitBlockStmt(const auto& e) {
        info("BlockStmt");
        Base::visitBlockStmt(e);
    }
    void visitCompoundStmt(const auto& e) {
        info("CompoundStmt", fmt::format("({} children)", e.childCount));
        Base::visitCompoundStmt(e);
    }

    void dispatchVisit(auto e) {
        indent += 2;
        stack.emplace_back(curContext, indent);
        curContext = stack.size() - 1;
        Base::dispatchVisit(e);
        curContext = stack[curContext].parent;
        indent -= 2;
    }

    void dump(auto e) {
        this->dispatchVisit(e);
        for (auto& ctx : stack)
            ctx.print();
        stack.clear();
    }
};

template<typename Impl>
using PersistentASTVisitorHelper = PersistentASTVisitor<Impl>;
using TemplateASTDumper = ASTDumper<TemplateASTVisitor>;
using PersistentASTDumper = ASTDumper<PersistentASTVisitorHelper>;

struct StmtDumper : StmtConsumerStub {
    template<typename Stmt>
    void consumeStmt(StmtConsumerBase&, Stmt stmt) {
        TemplateASTDumper dumper;
        dumper.dump(stmt);
    }
};
}
