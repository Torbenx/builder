#pragma once

#include "AST.hpp"

namespace shadergen::compressed {

// Common
template<auto callback>
struct ASTHolder {
    static constexpr auto ast = WrappedAST(callback);
};

// resolve node types
enum class NodeKind {
    Unknown,
    Stmt,
    ListEntry,
    VarDecl,
};
struct NodeType {
    NodeKind nodeKind = NodeKind::Unknown;
    StmtKind stmtKind = StmtKind::Stmt;
    constexpr NodeType() = default;
    constexpr NodeType(NodeKind nodeKind)
        : nodeKind(nodeKind), stmtKind(StmtKind::Stmt) { }
    constexpr NodeType(StmtKind stmtKind)
        : nodeKind(NodeKind::Stmt), stmtKind(stmtKind) { }
};

struct ResolvedNodes {
    NodeType nodes[100];
    int_t nodeCount = 0;
};

struct NodeResolver : ASTVisitor<NodeResolver> {
    ResolvedNodes nodes;
    constexpr void allocate(NodeType type) {
        nodes.nodes[nodes.nodeCount++] = type;
    }
    constexpr void visitStmt(const Stmt* stmt) {
        allocate(stmt->stmtKind);
    }
    constexpr void visitCompoundStmt(const CompoundStmt* stmt) {
        visitStmt(stmt);
        // nodes.nodeCount += stmt->entryCount;
        stmt->forEachEntry([&](const Stmt* stmt) {
            dispatchVisit(stmt);
            allocate(NodeKind::ListEntry);
        });
    }
    constexpr void visitVarDeclStmt(const VarDeclStmt* stmt) {
        visitStmt(stmt);
        allocate(NodeKind::VarDecl);
        allocate(NodeKind::ListEntry);
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
    constexpr T* allocateStmt(const T& in) {
        return std::construct_at(static_cast<T*>(storage[offset++]), in);
    }
    constexpr VarDecl* allocateVarDecl(const VarDecl& in) {
        return std::construct_at(static_cast<VarDecl*>(storage[offset++]), in);
    }
    template<typename T>
    constexpr void pushBackOnList(ASTList<T>& list, T* in) {
        auto* entry = std::construct_at(static_cast<ASTListEntry*>(storage[offset++]), ASTListEntry { in });
        list.pushBack(entry);
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
template<NodeType type>
struct NodeTypeLookup;

template<>
struct NodeTypeLookup<NodeKind::ListEntry> {
    using type = ASTListEntry;
};

template<>
struct NodeTypeLookup<NodeKind::VarDecl> {
    using type = VarDecl;
};

#define STMT_KIND(S)                     \
    template<>                           \
    struct NodeTypeLookup<StmtKind::S> { \
        using type = S;                  \
    };
ENUMERATE_STMT_KINDS(STMT_KIND)
#undef STMT_KIND

template<ResolvedNodes nodes, size_t... I>
static constexpr auto createTuple3(const AST& ast, std::index_sequence<I...>) {
    return FilledTuple<storage_t<typename NodeTypeLookup<nodes.nodes[I]>::type, I>...>(ast);
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
