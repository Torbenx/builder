#pragma once

#include "TemplateASTVisitor.hpp"
#include "context.hpp"
#include <variant>

namespace shadergen {

struct PersistentAST;

template<typename T>
struct NodeHandle {
    int_t index = -1;

    template<typename S>
    constexpr operator NodeHandle<S>() const requires std::is_base_of_v<S, T> { return { index }; }
    template<typename S>
    constexpr explicit operator NodeHandle<S>() const { return { index }; }

    constexpr explicit operator bool() const { return index != -1; }
};

struct StmtNode { };
struct ExprNode : StmtNode {
    TypeHandle m_type;
    constexpr TypeHandle type() const { return m_type; }
};
struct VarDeclStmtNode : StmtNode {
    TypeHandle type;
    DynamicName m_name;
    constexpr std::string_view name() const { return m_name.view(); }
};
struct VarExprNode : ExprNode {
    NodeHandle<VarDeclStmtNode> varDecl;
};
struct ElementwiseBinaryOperatorExprNode : ExprNode {
    BinaryOperatorEnum m_op;
    NodeHandle<ExprNode> left;
    NodeHandle<ExprNode> right;
    constexpr BinaryOperatorEnum op() const { return m_op; }
};
struct VectorByScalarBinaryOperatorExprNode : ExprNode {
    BinaryOperatorEnum m_op;
    NodeHandle<ExprNode> vector;
    NodeHandle<ExprNode> scalar;
    bool scalarIsLeft;
    constexpr BinaryOperatorEnum op() const { return m_op; }
    constexpr bool isScalarLeft() const { return scalarIsLeft; }
};
struct UnaryOperatorExprNode : ExprNode {
    UnaryOperatorEnum m_op;
    NodeHandle<ExprNode> subExpr;
    constexpr UnaryOperatorEnum op() const { return m_op; }
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
struct CompoundStmtNode : StmtNode {
    struct Entry {
        NodeHandle<StmtNode> stmt;
        NodeHandle<Entry> next;
    };
    NodeHandle<Entry> first;
    NodeHandle<Entry> last;
    int_t childCount = 0;

    constexpr void pushBack(PersistentAST& ast, NodeHandle<StmtNode> stmt);
    template<typename Callback>
    constexpr void forEachStmt(auto& ast, Callback&& callback) const;
};
struct BlockStmtNode : CompoundStmtNode {
    NodeHandle<BlockStmtNode> parent;
};
struct FieldAccessExprNode : ExprNode {
    NodeHandle<ExprNode> base;
    TypeHandle baseType;
    int_t m_fieldIndex = -1;
    constexpr int_t fieldIndex() const { return m_fieldIndex; }
};

using NodeStorage = std::variant<
    StmtNode,
    ExprNode,
    VarExprNode,
    ElementwiseBinaryOperatorExprNode,
    VectorByScalarBinaryOperatorExprNode,
    UnaryOperatorExprNode,
    AssignmentStmtNode,
    LiteralExprNode,
    VarDeclStmtNode,
    CompoundStmtNode,
    CompoundStmtNode::Entry,
    BlockStmtNode,
    FieldAccessExprNode>;

struct PersistentAST {
    StaticVector<NodeStorage, 100> nodes;
    StaticVector<NodeHandle<VarDeclStmtNode>, 100> varDecls;

    TypeContext typeContext;
    NodeHandle<BlockStmtNode> currentBlock;

    constexpr PersistentAST() {
        currentBlock = allocate(BlockStmtNode {});
    }

    template<typename T>
    constexpr NodeHandle<T> allocate(T node) {
        nodes.pushBack(node);
        return { nodes.size() - 1 };
    }

    template<typename T>
    constexpr NodeHandle<T> emitStmt(T stmt) {
        NodeHandle<T> node = allocate(stmt);
        this->node(currentBlock).pushBack(*this, node);
        return node;
    }

    template<typename T>
    constexpr const NodeStorage& nodeStorage(NodeHandle<T> handle) const {
        return nodes[handle.index];
    }
    template<typename T>
    constexpr NodeStorage& nodeStorage(NodeHandle<T> handle) {
        return nodes[handle.index];
    }
    template<typename T>
    constexpr T* nodeOrNull(NodeHandle<T> handle) {
        NodeStorage& ns = nodeStorage(handle);
        return std::visit([]<typename S>(S& node) -> T* {
            if constexpr (std::derived_from<S, T>)
                return &node;
            else
                return nullptr;
        },
            ns);
    }
    template<typename T>
    constexpr T& node(NodeHandle<T> handle) {
        T* ptr = nodeOrNull(handle);
        VERIFY(ShaderGen, ptr != nullptr);
        return *ptr;
    }
    template<typename T>
    constexpr const T* nodeOrNull(NodeHandle<T> handle) const {
        const NodeStorage& ns = nodeStorage(handle);
        return std::visit([]<typename S>(const S& node) -> const T* {
            if constexpr (std::derived_from<S, T>)
                return &node;
            else
                return nullptr;
        },
            ns);
    }
    template<typename T>
    constexpr const T& node(NodeHandle<T> handle) const {
        const T* ptr = nodeOrNull(handle);
        VERIFY(ShaderGen, ptr != nullptr);
        return *ptr;
    }

    struct NodeBuilder : TemplateASTVisitor<NodeBuilder> {
        PersistentAST& ast;

        constexpr NodeBuilder(PersistentAST& ast)
            : ast(ast) { }

        constexpr ExprNode visitExpr(auto e) {
            return { {}, ast.typeContext.findType(e.type()) };
        }
        constexpr VarExprNode visitVarExpr(auto e) {
            return { visitExpr(e), ast.varDecls[e.varHandle.index] };
        }
        constexpr ElementwiseBinaryOperatorExprNode visitElementwiseBinaryOperatorExpr(auto e) {
            return {
                visitExpr(e),
                e.op(),
                ast.allocate(dispatchVisit(e.left)),
                ast.allocate(dispatchVisit(e.right))
            };
        }
        constexpr VectorByScalarBinaryOperatorExprNode visitVectorByScalarBinaryOperatorExpr(auto e) {
            return {
                visitExpr(e),
                e.op(),
                ast.allocate(dispatchVisit(e.vector)),
                ast.allocate(dispatchVisit(e.scalar)),
                e.isScalarLeft()
            };
        }
        constexpr UnaryOperatorExprNode visitUnaryOperatorExpr(auto e) {
            return {
                visitExpr(e),
                e.op(),
                ast.allocate(dispatchVisit(e.subExpr))
            };
        }
        constexpr AssignmentStmtNode visitAssignmentStmt(auto s) {
            return {
                {},
                ast.allocate(dispatchVisit(s.left)),
                ast.allocate(dispatchVisit(s.right))
            };
        }
        constexpr LiteralExprNode visitLiteralExpr(auto e) {
            return { visitExpr(e), e.value };
        }
        constexpr FieldAccessExprNode visitFieldAccessExpr(auto e) {
            return {
                visitExpr(e),
                ast.allocate(dispatchVisit(e.base)),
                ast.typeContext.findType(e.base.type()),
                e.fieldIndex()
            };
        }
    };

    template<typename T, fixed_string name>
    constexpr void declareVariable(StmtConsumerBase&, VarHandle handle) {
        EXPECT(ShaderGen, handle.index, varDecls.size());
        varDecls.pushBack(emitStmt(VarDeclStmtNode { {}, typeContext.findType<T>(), name }));
    }

    template<typename Stmt>
    constexpr void consumeStmt(StmtConsumerBase&, Stmt stmt) {
        NodeBuilder builder { *this };
        emitStmt(builder.dispatchVisit(stmt));
    }

    constexpr void pushBlock() {
        currentBlock = emitStmt(BlockStmtNode { {}, currentBlock });
    }
    constexpr void popBlock() {
        NodeHandle<BlockStmtNode> tmp = node(currentBlock).parent;
        currentBlock = tmp;
    }
    constexpr void beginBlock(StmtConsumerBase&) { pushBlock(); }
    constexpr void endBlock(StmtConsumerBase&) { popBlock(); }
};

constexpr void CompoundStmtNode::pushBack(PersistentAST& ast, NodeHandle<StmtNode> stmt) {
    NodeHandle<Entry> newEntry = ast.allocate(Entry { stmt });
    if (last)
        ast.node(last).next = newEntry;
    else
        first = newEntry;
    last = newEntry;
    childCount += 1;
}

template<typename Callback>
constexpr void CompoundStmtNode::forEachStmt(auto& ast, Callback&& callback) const {
    NodeHandle<Entry> entry = first;
    while (entry) {
        auto& e = ast.node(entry);
        callback(e.stmt);
        entry = e.next;
    }
}

}
