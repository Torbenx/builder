#pragma once

#include "AST.hpp"
#include <tuple>

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
struct TupleLookupTableBase {
    ASTNode** table;
    constexpr ASTNode* get(int_t i) const { return table[i]; }
};
template<typename... Types>
struct TupleLookupTable : TupleLookupTableBase {
    ASTNode* nodes[sizeof...(Types)];
    template<size_t... I>
    constexpr void initialize(std::tuple<Types...>& tuple, std::index_sequence<I...>) {
        ((nodes[I] = &std::get<I>(tuple).s), ...);
    }
    constexpr TupleLookupTable(std::tuple<Types...>& tuple)
        : TupleLookupTableBase { nodes } {
        initialize(tuple, std::index_sequence_for<Types...>());
    }
};

struct Compressor : ASTTransform<Compressor> {
    TupleLookupTableBase tuple;
    int_t offset = 0;
    template<typename T>
    constexpr T* allocate(const T& in) {
        return std::construct_at(static_cast<T*>(tuple.get(offset++)), in);
    }
};

template<typename... Types>
struct FilledTuple : std::tuple<Types...> {
    constexpr FilledTuple(const AST& ast) {
        TupleLookupTable table(*this);
        Compressor comp { {}, table };
        comp.dispatchTransform(ast.currentBlock);
    }
};

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
static constexpr auto createTuple_helper(const AST& ast, std::integer_sequence<int_t, I...>) {
    return FilledTuple<typename NodeKindToType<nodes.nodes[I]>::type...>(ast);
}

template<auto callback>
static constexpr auto createTuple() {
    constexpr auto nodes = resolveNodes(WrappedAST(callback));
    return createTuple_helper<nodes>(WrappedAST(callback), std::make_integer_sequence<int_t, nodes.nodeCount>());
}

}
