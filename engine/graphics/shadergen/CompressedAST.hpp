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
    constexpr void visitStmt(const Stmt& stmt) {
        nodes.nodes[nodes.nodeCount++] = stmt.stmtKind;
    }
    constexpr void visitCompoundStmt(const CompoundStmt& stmt) {
        visitStmt(stmt);
        nodes.nodeCount += stmt.entryCount;
        stmt.forEachStmt([&](const Stmt& stmt) {
            dispatchVisit(stmt);
        });
    }
};

static constexpr ResolvedNodes resolveNodes(const AST& ast) {
    NodeResolver res;
    res.dispatchVisit(*ast.currentBlock);
    return res.nodes;
}

// create tuple
template<NodeKind kind>
struct NodeKindToType;
union StmtListEntryNoInit {
    constexpr StmtListEntryNoInit() { }
    StmtListEntry s;
};
template<>
struct NodeKindToType<NodeKind {}> {
    using type = StmtListEntryNoInit;
};
#define STMT_KIND(S)                     \
    union S##NoInit {                    \
        constexpr S##NoInit() { }        \
        S s;                             \
    };                                   \
    template<>                           \
    struct NodeKindToType<StmtKind::S> { \
        using type = S##NoInit;          \
    };
ENUMERATE_STMT_KINDS(STMT_KIND)
#undef STMT_KIND

template<ResolvedNodes nodes, int_t... I>
static constexpr auto createTuple_helper(std::integer_sequence<int_t, I...>) {
    return std::tuple<typename NodeKindToType<nodes.nodes[I]>::type...>();
}

template<typename Holder>
static constexpr auto createTuple() {
    constexpr auto nodes = resolveNodes(Holder::ast);
    return createTuple_helper<nodes>(std::make_integer_sequence<int_t, nodes.nodeCount>());
}

// fill tuple
struct TupleLookupTableBase {
    ASTNode*** table;
    constexpr ASTNode* get(int_t i) const { return (*table)[i]; }
};
template<typename... Types>
struct TupleLookupTable : TupleLookupTableBase {
    ASTNode* nodes[sizeof...(Types)];
    template<size_t... I>
    constexpr void initialize(std::tuple<Types...>& tuple, std::index_sequence<I...>) {
        ((nodes[I] = &std::get<I>(tuple)), ...);
    }
    constexpr TupleLookupTable(std::tuple<Types...>& tuple)
        : TupleLookupTableBase { &nodes } {
        initialize(tuple, std::index_sequence_for<Types...>());
    }
};

}
