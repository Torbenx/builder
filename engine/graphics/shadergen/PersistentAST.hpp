#pragma once

#include "context.hpp"
#include <variant>

namespace shadergen {

template<typename T>
struct NodeHandle {
    int_t index = -1;

    template<typename S>
    constexpr operator NodeHandle<S>() requires std::is_base_of_v<S, T> { return { index }; }
    template<typename S>
    constexpr explicit operator NodeHandle<S>() { return { index }; }
};

struct ExprNode {
    TypeHandle m_type;
    constexpr TypeHandle type() { return m_type; }
};
struct VarExprNode : ExprNode {
    VarHandle variable;
};
struct ElementwiseBinaryOperatorExprNode : ExprNode {
    BinaryOperatorEnum m_op;
    NodeHandle<ExprNode> m_left;
    NodeHandle<ExprNode> m_right;
    constexpr BinaryOperatorEnum op() { return m_op; }
    constexpr NodeHandle<ExprNode> left() { return m_left; }
    constexpr NodeHandle<ExprNode> right() { return m_right; }
};
struct VectorByScalarBinaryOperatorExprNode : ExprNode {
    BinaryOperatorEnum m_op;
    NodeHandle<ExprNode> m_vector;
    NodeHandle<ExprNode> m_scalar;
    bool scalarIsLeft;
    constexpr BinaryOperatorEnum op() { return m_op; }
    constexpr NodeHandle<ExprNode> vector() { return m_vector; }
    constexpr NodeHandle<ExprNode> scalar() { return m_scalar; }
    constexpr bool isScalarLeft() { return scalarIsLeft; }
};
struct UnaryOperatorExprNode : ExprNode {
    UnaryOperatorEnum m_op;
    NodeHandle<ExprNode> m_subExpr;
    constexpr UnaryOperatorEnum op() { return m_op; }
    constexpr NodeHandle<ExprNode> subExpr() { return m_subExpr; }
};

using NodeStorage = std::variant<
    ExprNode,
    VarExprNode,
    ElementwiseBinaryOperatorExprNode,
    VectorByScalarBinaryOperatorExprNode,
    UnaryOperatorExprNode>;

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

    TypeContext typeContext;
    VarDeclContext varContext;

    struct Builder;
};

}
