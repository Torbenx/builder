#pragma once

#include "AST.hpp"

namespace shadergen::compressed {

// Common
template<auto callback>
struct ASTHolder {
    static constexpr auto ast = WrappedAST(callback);
};

// resolve node types
struct NodeKind {
    StmtKind kind = StmtKind::Stmt;
    bool isListStmtEntry = true;
    constexpr NodeKind() = default;
    constexpr NodeKind(StmtKind kind)
        : kind(kind), isListStmtEntry(false) { }
};

struct ResolvedNodes {
    NodeKind nodes[100];
    int_t nodeCount = 0;
};

struct NodeResolver : ASTVisitor<NodeResolver> {
    ResolvedNodes nodes;
    constexpr void visitStmt(const Stmt* stmt) {
        nodes.nodes[nodes.nodeCount++] = stmt->stmtKind;
    }
    constexpr void visitCompoundStmt(const CompoundStmt* stmt) {
        visitStmt(stmt);
        // nodes.nodeCount += stmt->entryCount;
        stmt->forEachStmt([&](const Stmt* stmt) {
            dispatchVisit(stmt);
            nodes.nodeCount += 1;
        });
    }
};

static constexpr ResolvedNodes resolveNodes(const AST& ast) {
    NodeResolver res;
    res.dispatchVisit(ast.currentBlock);
    return res.nodes;
}

// fill tuple
struct Compressor : ASTTransform<Compressor> {
    ASTNode** storage;
    int_t offset = 0;
    template<typename T>
    constexpr T* allocate(const T& in) {
        return std::construct_at(static_cast<T*>(storage[offset++]), in);
    }
};

template<typename T, size_t i>
struct storage_t {
    union U {
        T node;
        constexpr U() { }
    } u;
    constexpr ASTNode* node() { return &u.node; }
};

template<typename... Types>
struct FilledTuple : Types... {
    constexpr FilledTuple(const AST& ast) {
        ASTNode* nodes[] = { Types::node()... };
        Compressor comp { {}, nodes };
        comp.dispatchTransform(ast.currentBlock);
    }
};

// create tuple
template<NodeKind kind>
struct NodeKindToType;
template<>
struct NodeKindToType<NodeKind {}> {
    using type = StmtListEntry;
};
#define STMT_KIND(S)                     \
    template<>                           \
    struct NodeKindToType<StmtKind::S> { \
        using type = S;                  \
    };
ENUMERATE_STMT_KINDS(STMT_KIND)
#undef STMT_KIND

template<ResolvedNodes nodes, size_t... I>
static constexpr auto createTuple3(const AST& ast, std::index_sequence<I...>) {
    return FilledTuple<storage_t<typename NodeKindToType<nodes.nodes[I]>::type, I>...>(ast);
}

template<typename Holder>
static constexpr auto createTuple2() {
    constexpr auto nodes = resolveNodes(Holder::ast);
    return createTuple3<nodes>(Holder::ast, std::make_index_sequence<nodes.nodeCount>());
}

template<auto callback>
static constexpr auto createTuple() {
    constexpr auto nodes = resolveNodes(WrappedAST(callback));
    return createTuple3<nodes>(WrappedAST(callback), std::make_index_sequence<nodes.nodeCount>());
}

}
