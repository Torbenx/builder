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
// must match builtinTypeInfos
using LiternalStorage = std::variant<
    bool, vec<bool, 2>, vec<bool, 3>, vec<bool, 4>,
    int, vec<int, 2>, vec<int, 3>, vec<int, 4>,
    float, vec<float, 2>, vec<float, 3>, vec<float, 4>>;

struct LiteralExprNode : ExprNode {
    LiternalStorage value;
};

using NodeStorage = std::variant<
    ExprNode,
    VarExprNode,
    ElementwiseBinaryOperatorExprNode,
    VectorByScalarBinaryOperatorExprNode,
    UnaryOperatorExprNode,
    AssignmentStmtNode,
    LiteralExprNode>;

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
};

struct PersistentAST::Builder : TemplateASTVisitor<Builder> {
    PersistentAST& ast;
    constexpr Builder(PersistentAST& ast)
        : ast(ast) { }

    constexpr ExprNode visitExpr(auto e) {
        return { {}, ast.typeContext.findType(e.type()) };
    }
    constexpr VarExprNode visitVarExpr(auto e) {
        return { visitExpr(e), e.varDecl };
    }
    constexpr ElementwiseBinaryOperatorExprNode visitElementwiseBinaryOperatorExpr(auto e) {
        return {
            visitExpr(e),
            e.op(),
            ast.emit(dispatchVisit(e.left)),
            ast.emit(dispatchVisit(e.right))
        };
    }
    constexpr VectorByScalarBinaryOperatorExprNode visitVectorByScalarBinaryOperatorExpr(auto e) {
        return {
            visitExpr(e),
            e.op(),
            ast.emit(dispatchVisit(e.vector)),
            ast.emit(dispatchVisit(e.scalar)),
            e.isScalarLeft()
        };
    }
    constexpr UnaryOperatorExprNode visitUnaryOperatorExpr(auto e) {
        return {
            visitExpr(e),
            e.op(),
            ast.emit(dispatchVisit(e.subExpr))
        };
    }
    constexpr AssignmentStmtNode visitAssignmentStmt(auto s) {
        return {
            {},
            ast.emit(dispatchVisit(s.left)),
            ast.emit(dispatchVisit(s.right))
        };
    }
    constexpr LiteralExprNode visitLiteralExpr(auto e) {
        return { visitExpr(e), e.value };
    }
};

}
