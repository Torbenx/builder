#pragma once

#include "AST.hpp"

namespace shadergen::compressed {

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

static constexpr ResolvedNodes resolveNodes(const Stmt* root) {
    NodeResolver res;
    res.dispatchVisit(root);
    return res.nodes;
}

// fill tuple
struct CompressedASTStorageAllocator {
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
struct Compressor : ASTTransform<Compressor, CompressedASTStorageAllocator> {
    constexpr Compressor(CompressedASTStorageAllocator& alloc)
        : ASTTransform<Compressor, CompressedASTStorageAllocator>(&alloc) { }
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
    Stmt* root = nullptr;
    constexpr FilledTuple(const Stmt* root) {
        ASTNode* nodes[] = { Types::node()... };
        CompressedASTStorageAllocator alloc { nodes };
        Compressor comp { alloc };
        this->root = comp.dispatchTransform(root);
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

template<auto callback>
struct ASTHolder {
    static constexpr auto ast = BuildDriver(callback);
};

template<ResolvedNodes nodes, size_t... I>
static constexpr auto createTuple3(const Stmt* root, std::index_sequence<I...>) {
    return FilledTuple<storage_t<typename NodeTypeLookup<nodes.nodes[I]>::type, I>...>(root);
}

template<typename Holder>
static constexpr auto createTuple2() {
    constexpr auto nodes = resolveNodes(Holder::ast.builder.currentBlock);
    return createTuple3<nodes>(Holder::ast.builder.currentBlock, std::make_index_sequence<nodes.nodeCount>());
}

template<auto callback>
static constexpr auto createTuple() {
    constexpr auto nodes = resolveNodes(BuildDriver(callback).builder.currentBlock);
    return createTuple3<nodes>(BuildDriver(callback).builder.currentBlock, std::make_index_sequence<nodes.nodeCount>());
}

}
