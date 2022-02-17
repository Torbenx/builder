#pragma once

#include "util/log.hpp"
#include "util/reflection.hpp"
#include "util/types.hpp"
#include "util/uninitialized.hpp"
#include <string>
#include <tuple>
#include <vector>
#include <bit>
#include <limits>

#include "ASTChecker.hpp"
#include "PersistentASTVisitor.hpp"

namespace shadergen {

template<typename... Consumers>
struct StmtConsumerFrontEnd {
    std::tuple<Consumers...> consumers;
    static constexpr int_t BLOCK_SHIFT = 8;
    using blockidx_t = size_t;
    static constexpr blockidx_t SUB_BLOCK_MASK = ((blockidx_t)1 << BLOCK_SHIFT) - 1;
    size_t localSubBlockCount = 0;
    blockidx_t currentBlockIndex = 1;

    template<typename Callback>
    constexpr void forEachConsumer(Callback&& callback) {
        (callback(std::get<Consumers>(consumers)), ...);
    }

    template<typename Stmt>
    constexpr void operator,(Stmt stmt) {
        forEachConsumer([=](auto consumer) { consumer.consumeStmt(stmt); });
    }
    template<typename T, fixed_string name>
    constexpr VarExpr<T, name> declareVariable() {
        return {}
    }
    constexpr void beginBlock() {
        currentBlockIndex <<= BLOCK_SHIFT;
        currentBlockIndex |= localSubBlockCount;
        localSubBlockCount = 0;
        forEachConsumer([](auto consumer) { consumer.beginBlock(); });
    }
    constexpr void endBlock() {
        forEachConsumer([](auto consumer) { consumer.endBlock(); });
        localSubBlockCount = (currentBlockIndex & SUB_BLOCK_MASK) + 1;
        currentBlockIndex >>= BLOCK_SHIFT;
    }
    static constexpr bool isSubBlock(blockidx_t parentIndex, blockidx_t childIndex) {
        int_t parentDepth = (std::numeric_limits<blockidx_t>::digits - std::countl_zero(parentIndex) - 1) / BLOCK_SHIFT;
        int_t childDepth = (std::numeric_limits<blockidx_t>::digits - std::countl_zero(childIndex) - 1) / BLOCK_SHIFT;
        if (parentDepth > childDepth)
            return false;
        blockidx_t relChildIndex = childIndex >> ((childDepth - parentDepth) * BLOCK_SHIFT);
        return relChildIndex == parentIndex;
    }
};

template<template<typename> typename Visitor>
struct ASTDumper : Visitor<ASTDumper<Visitor>> {
    using Visitor<ASTDumper>::Visitor;
    int_t indent = 0;
    std::string nodeType;
    std::vector<std::string> infos;
    template<typename... Args>
    void print() {
        std::string out = "";
        for (auto info : infos) {
            out += ' ';
            out += info;
        }
        log("{:{}} {}{}", "", indent, nodeType, out);
        nodeType = "";
        infos.clear();
    }

    template<typename... Args>
    void info(std::string type, Args&&... args) {
        if (nodeType.empty())
            nodeType = type;
        (infos.emplace_back(std::forward<Args>(args)), ...);
    }

    void visitExpr(auto e) {
        info("Expr", fmt::format("{}", this->getType(e)));
    }
    void visitVarExpr(auto e) {
        info("VarExpr", fmt::format("\"{}\"", this->getVarDecl(e).name()));
        visitExpr(e);
        print();
    }
    void visitElementwiseBinaryOperatorExpr(auto e) {
        info("ElementwiseBinaryOperatorExpr", fmt::format("'{}'", toString(e.op())));
        visitExpr(e);
        print();
        visit(e.left);
        visit(e.right);
    }
    void visitVectorByScalarBinaryOperatorExpr(auto e) {
        info("VectorByScalarBinaryOperatorExpr", fmt::format("'{}'", toString(e.op())));
        visitExpr(e);
        print();
        visit(e.vector);
        visit(e.scalar);
    }
    void visitUnaryOperatorExpr(auto e) {
        info("UnaryOperatorExpr", fmt::format("'{}'", toString(e.op())));
        visitExpr(e);
        print();
        visit(e.subExpr);
    }
    void visitAssignmentStmt(auto e) {
        info("AssignmentStmt");
        print();
        visit(e.left);
        visit(e.right);
    }
    void visitLiteralExpr(auto e) {
        info("LiteralExpr");
        visitExpr(e);
        print();
    }

    void dump(auto e) {
        this->dispatchVisit(e);
    }

    void visit(auto e) {
        indent += 2;
        dump(e);
        indent -= 2;
    }
};

template<typename Impl>
using PersistentASTVisitorHelper = PersistentASTVisitor<Impl>;
using TemplateASTDumper = ASTDumper<TemplateASTVisitor>;
using PersistentASTDumper = ASTDumper<PersistentASTVisitorHelper>;

}
