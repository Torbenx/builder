#pragma once

#include "TemplateASTVisitor.hpp"
#include "context.hpp"
#include <variant>

namespace shadergen {

template<typename T>
struct NodeHandle {
    int_t index = -1;

    template<typename S>
    constexpr operator NodeHandle<S>() const requires std::is_base_of_v<S, T> { return { index }; }
    template<typename S>
    constexpr explicit operator NodeHandle<S>() const { return { index }; }
};

struct StmtNode { };
struct ExprNode : StmtNode {
    TypeHandle type;
};
struct VarExprNode : ExprNode {
    VarHandle varDecl;
};
struct ElementwiseBinaryOperatorExprNode : ExprNode {
    BinaryOperatorEnum m_op;
    NodeHandle<ExprNode> left;
    NodeHandle<ExprNode> right;
    constexpr BinaryOperatorEnum op() { return m_op; }
};
struct VectorByScalarBinaryOperatorExprNode : ExprNode {
    BinaryOperatorEnum m_op;
    NodeHandle<ExprNode> vector;
    NodeHandle<ExprNode> scalar;
    bool scalarIsLeft;
    constexpr BinaryOperatorEnum op() { return m_op; }
    constexpr bool isScalarLeft() { return scalarIsLeft; }
};
struct UnaryOperatorExprNode : ExprNode {
    UnaryOperatorEnum m_op;
    NodeHandle<ExprNode> subExpr;
    constexpr UnaryOperatorEnum op() { return m_op; }
};
struct AssignmentStmtNode : StmtNode {
    NodeHandle<ExprNode> left;
    NodeHandle<ExprNode> right;
};

using NodeStorage = std::variant<
    ExprNode,
    VarExprNode,
    ElementwiseBinaryOperatorExprNode,
    VectorByScalarBinaryOperatorExprNode,
    UnaryOperatorExprNode,
    AssignmentStmtNode>;

struct PersistentAST {
    StaticVector<NodeStorage, 100> nodes;

    template<typename T>
    constexpr NodeHandle<T> emit(T node) {
        nodes.pushBack(node);
        return { nodes.size() - 1 };
    }

    template<typename T>
    constexpr const NodeStorage& node(NodeHandle<T> handle) const {
        return nodes[handle.index];
    }
    template<typename T>
    constexpr NodeStorage& node(NodeHandle<T> handle) {
        return nodes[handle.index];
    }

    TypeContext typeContext;
    VarDeclContext varContext;

    struct Builder;
    struct CheckExprUsage;
};

}
