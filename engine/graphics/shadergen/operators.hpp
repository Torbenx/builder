#pragma once

#include "vectors.hpp"

namespace shadergen {

// Unary operators
enum class UnaryOperator {
    NEGATE,
    NOT,
};
static constexpr bool isArithmeticOp(UnaryOperator op) { return op == UnaryOperator::NEGATE; }
static constexpr bool isLogicalOp(UnaryOperator op) { return op == UnaryOperator::NOT; }
static constexpr std::string_view toString(UnaryOperator op) {
    switch (op) {
    case UnaryOperator::NEGATE:
        return "-";
    case UnaryOperator::NOT:
        return "!";
    default:
        return "??";
    }
}
struct UnaryOperatorExpr : Expr {
    UnaryOperator op;
    const Expr* subExpr;
    constexpr UnaryOperatorExpr(UnaryOperator op, const Expr* subExpr)
        : Expr(StmtKind::UnaryOperatorExpr), op(op), subExpr(subExpr) { }
};
namespace code {
    constexpr wrapped_scalar::impl<UnaryOperatorExpr> operator-(const wrapped_scalar& expr) { return { { UnaryOperator::NEGATE, expr.wrappedExpr } }; }
    constexpr wrapped_vector::impl<UnaryOperatorExpr> operator-(const wrapped_vector& expr) { return { { UnaryOperator::NEGATE, expr.wrappedExpr } }; }
    constexpr wrapped_scalar::impl<UnaryOperatorExpr> operator!(const wrapped_scalar& expr) { return { { UnaryOperator::NOT, expr.wrappedExpr } }; }
    constexpr wrapped_vector::impl<UnaryOperatorExpr> operator!(const wrapped_vector& expr) { return { { UnaryOperator::NOT, expr.wrappedExpr } }; }
}

// Binary operators
enum class BinaryOperator {
    PLUS,
    MINUS,
    MULTIPLY,
    DIVIDE,
    CMP_LE,
    CMP_L,
    CMP_EQ,
    CMP_GE,
    CMP_G,
    CMP_NE,
    AND,
    OR,
};
static constexpr bool isArithmeticOp(BinaryOperator op) {
    return op == BinaryOperator::PLUS
        || op == BinaryOperator::MINUS
        || op == BinaryOperator::MULTIPLY
        || op == BinaryOperator::DIVIDE;
}
static constexpr bool isLogicalOp(BinaryOperator op) {
    return op == BinaryOperator::AND
        || op == BinaryOperator::OR;
}
static constexpr bool isComparisonOp(BinaryOperator op) {
    return op == BinaryOperator::CMP_LE
        || op == BinaryOperator::CMP_L
        || op == BinaryOperator::CMP_EQ
        || op == BinaryOperator::CMP_GE
        || op == BinaryOperator::CMP_G
        || op == BinaryOperator::CMP_NE;
}
static constexpr std::string_view toString(BinaryOperator op) {
    switch (op) {
    case BinaryOperator::PLUS:
        return "+";
    case BinaryOperator::MINUS:
        return "-";
    case BinaryOperator::MULTIPLY:
        return "*";
    case BinaryOperator::DIVIDE:
        return "/";
    case BinaryOperator::CMP_LE:
        return "<=";
    case BinaryOperator::CMP_L:
        return "<";
    case BinaryOperator::CMP_EQ:
        return "==";
    case BinaryOperator::CMP_GE:
        return ">=";
    case BinaryOperator::CMP_G:
        return ">";
    case BinaryOperator::CMP_NE:
        return "!=";
    case BinaryOperator::AND:
        return "&&";
    case BinaryOperator::OR:
        return "||";
    default:
        return "??";
    }
}

// Elementwise operators
struct ElementwiseOperatorExpr : Expr {
    BinaryOperator op;
    const Expr* left;
    const Expr* right;
    constexpr ElementwiseOperatorExpr(BinaryOperator op, const Expr* left, const Expr* right)
        : Expr(StmtKind::ElementwiseOperatorExpr), op(op), left(left), right(right) { }
};
namespace code {
    constexpr wrapped_scalar::impl<ElementwiseOperatorExpr> operator+(const wrapped_scalar& left, const wrapped_scalar& right) { return { { BinaryOperator::PLUS, left.wrappedExpr, right.wrappedExpr } }; }
    constexpr wrapped_scalar::impl<ElementwiseOperatorExpr> operator-(const wrapped_scalar& left, const wrapped_scalar& right) { return { { BinaryOperator::MINUS, left.wrappedExpr, right.wrappedExpr } }; }
    constexpr wrapped_scalar::impl<ElementwiseOperatorExpr> operator*(const wrapped_scalar& left, const wrapped_scalar& right) { return { { BinaryOperator::MULTIPLY, left.wrappedExpr, right.wrappedExpr } }; }
    constexpr wrapped_scalar::impl<ElementwiseOperatorExpr> operator/(const wrapped_scalar& left, const wrapped_scalar& right) { return { { BinaryOperator::DIVIDE, left.wrappedExpr, right.wrappedExpr } }; }

    constexpr wrapped_scalar::impl<ElementwiseOperatorExpr> operator&&(const wrapped_scalar& left, const wrapped_scalar& right) { return { { BinaryOperator::AND, left.wrappedExpr, right.wrappedExpr } }; }
    constexpr wrapped_scalar::impl<ElementwiseOperatorExpr> operator||(const wrapped_scalar& left, const wrapped_scalar& right) { return { { BinaryOperator::OR, left.wrappedExpr, right.wrappedExpr } }; }

    constexpr wrapped_scalar::impl<ElementwiseOperatorExpr> operator<(const wrapped_scalar& left, const wrapped_scalar& right) { return { { BinaryOperator::CMP_L, left.wrappedExpr, right.wrappedExpr } }; }
    constexpr wrapped_scalar::impl<ElementwiseOperatorExpr> operator<=(const wrapped_scalar& left, const wrapped_scalar& right) { return { { BinaryOperator::CMP_LE, left.wrappedExpr, right.wrappedExpr } }; }
    constexpr wrapped_scalar::impl<ElementwiseOperatorExpr> operator==(const wrapped_scalar& left, const wrapped_scalar& right) { return { { BinaryOperator::CMP_EQ, left.wrappedExpr, right.wrappedExpr } }; }
    constexpr wrapped_scalar::impl<ElementwiseOperatorExpr> operator>=(const wrapped_scalar& left, const wrapped_scalar& right) { return { { BinaryOperator::CMP_GE, left.wrappedExpr, right.wrappedExpr } }; }
    constexpr wrapped_scalar::impl<ElementwiseOperatorExpr> operator>(const wrapped_scalar& left, const wrapped_scalar& right) { return { { BinaryOperator::CMP_G, left.wrappedExpr, right.wrappedExpr } }; }
    constexpr wrapped_scalar::impl<ElementwiseOperatorExpr> operator!=(const wrapped_scalar& left, const wrapped_scalar& right) { return { { BinaryOperator::CMP_NE, left.wrappedExpr, right.wrappedExpr } }; }

    constexpr wrapped_vector::impl<ElementwiseOperatorExpr> operator+(const wrapped_vector& left, const wrapped_vector& right) { return { { BinaryOperator::PLUS, left.wrappedExpr, right.wrappedExpr } }; }
    constexpr wrapped_vector::impl<ElementwiseOperatorExpr> operator-(const wrapped_vector& left, const wrapped_vector& right) { return { { BinaryOperator::MINUS, left.wrappedExpr, right.wrappedExpr } }; }
    constexpr wrapped_vector::impl<ElementwiseOperatorExpr> operator*(const wrapped_vector& left, const wrapped_vector& right) { return { { BinaryOperator::MULTIPLY, left.wrappedExpr, right.wrappedExpr } }; }
    constexpr wrapped_vector::impl<ElementwiseOperatorExpr> operator/(const wrapped_vector& left, const wrapped_vector& right) { return { { BinaryOperator::DIVIDE, left.wrappedExpr, right.wrappedExpr } }; }

    constexpr wrapped_vector::impl<ElementwiseOperatorExpr> operator&&(const wrapped_vector& left, const wrapped_vector& right) { return { { BinaryOperator::AND, left.wrappedExpr, right.wrappedExpr } }; }
    constexpr wrapped_vector::impl<ElementwiseOperatorExpr> operator||(const wrapped_vector& left, const wrapped_vector& right) { return { { BinaryOperator::OR, left.wrappedExpr, right.wrappedExpr } }; }

    constexpr wrapped_vector::impl<ElementwiseOperatorExpr> operator<(const wrapped_vector& left, const wrapped_vector& right) { return { { BinaryOperator::CMP_L, left.wrappedExpr, right.wrappedExpr } }; }
    constexpr wrapped_vector::impl<ElementwiseOperatorExpr> operator<=(const wrapped_vector& left, const wrapped_vector& right) { return { { BinaryOperator::CMP_LE, left.wrappedExpr, right.wrappedExpr } }; }
    constexpr wrapped_vector::impl<ElementwiseOperatorExpr> operator==(const wrapped_vector& left, const wrapped_vector& right) { return { { BinaryOperator::CMP_EQ, left.wrappedExpr, right.wrappedExpr } }; }
    constexpr wrapped_vector::impl<ElementwiseOperatorExpr> operator>=(const wrapped_vector& left, const wrapped_vector& right) { return { { BinaryOperator::CMP_GE, left.wrappedExpr, right.wrappedExpr } }; }
    constexpr wrapped_vector::impl<ElementwiseOperatorExpr> operator>(const wrapped_vector& left, const wrapped_vector& right) { return { { BinaryOperator::CMP_G, left.wrappedExpr, right.wrappedExpr } }; }
    constexpr wrapped_vector::impl<ElementwiseOperatorExpr> operator!=(const wrapped_vector& left, const wrapped_vector& right) { return { { BinaryOperator::CMP_NE, left.wrappedExpr, right.wrappedExpr } }; }
}

// Vector by Scalar operators
struct VectorByScalarOperatorExpr : Expr {
    BinaryOperator op;
    const Expr* vector;
    const Expr* scalar;
    bool isVectorLeft;
    constexpr VectorByScalarOperatorExpr(BinaryOperator op, const Expr* vector, const Expr* scalar, bool isVectorLeft)
        : Expr(StmtKind::VectorByScalarOperatorExpr), op(op), vector(vector), scalar(scalar), isVectorLeft(isVectorLeft) { }
};
namespace code {
    constexpr wrapped_vector::impl<VectorByScalarOperatorExpr> operator+(const wrapped_vector& left, const wrapped_scalar& right) { return { { BinaryOperator::PLUS, left.wrappedExpr, right.wrappedExpr, true } }; }
    constexpr wrapped_vector::impl<VectorByScalarOperatorExpr> operator-(const wrapped_vector& left, const wrapped_scalar& right) { return { { BinaryOperator::MINUS, left.wrappedExpr, right.wrappedExpr, true } }; }
    constexpr wrapped_vector::impl<VectorByScalarOperatorExpr> operator*(const wrapped_vector& left, const wrapped_scalar& right) { return { { BinaryOperator::MULTIPLY, left.wrappedExpr, right.wrappedExpr, true } }; }
    constexpr wrapped_vector::impl<VectorByScalarOperatorExpr> operator/(const wrapped_vector& left, const wrapped_scalar& right) { return { { BinaryOperator::DIVIDE, left.wrappedExpr, right.wrappedExpr, true } }; }

    constexpr wrapped_vector::impl<VectorByScalarOperatorExpr> operator+(const wrapped_scalar& left, const wrapped_vector& right) { return { { BinaryOperator::PLUS, right.wrappedExpr, left.wrappedExpr, false } }; }
    constexpr wrapped_vector::impl<VectorByScalarOperatorExpr> operator-(const wrapped_scalar& left, const wrapped_vector& right) { return { { BinaryOperator::MINUS, right.wrappedExpr, left.wrappedExpr, false } }; }
    constexpr wrapped_vector::impl<VectorByScalarOperatorExpr> operator*(const wrapped_scalar& left, const wrapped_vector& right) { return { { BinaryOperator::MULTIPLY, right.wrappedExpr, left.wrappedExpr, false } }; }
    constexpr wrapped_vector::impl<VectorByScalarOperatorExpr> operator/(const wrapped_scalar& left, const wrapped_vector& right) { return { { BinaryOperator::DIVIDE, right.wrappedExpr, left.wrappedExpr, false } }; }
}

// Updates
struct UpdateStmt : Stmt {
    BinaryOperator op;
    const Expr* left;
    const Expr* right;
    constexpr UpdateStmt(BinaryOperator op, const Expr* left, const Expr* right)
        : Stmt(StmtKind::UpdateStmt), op(op), left(left), right(right) { }
};
namespace code {
    constexpr UpdateStmt operator+=(const wrapped_scalar& left, const wrapped_scalar& right) { return { BinaryOperator::PLUS, left.wrappedExpr, right.wrappedExpr }; }
    constexpr UpdateStmt operator-=(const wrapped_scalar& left, const wrapped_scalar& right) { return { BinaryOperator::MINUS, left.wrappedExpr, right.wrappedExpr }; }
    constexpr UpdateStmt operator*=(const wrapped_scalar& left, const wrapped_scalar& right) { return { BinaryOperator::MULTIPLY, left.wrappedExpr, right.wrappedExpr }; }
    constexpr UpdateStmt operator/=(const wrapped_scalar& left, const wrapped_scalar& right) { return { BinaryOperator::DIVIDE, left.wrappedExpr, right.wrappedExpr }; }

    constexpr UpdateStmt operator+=(const wrapped_vector& left, const wrapped_scalar& right) { return { BinaryOperator::PLUS, left.wrappedExpr, right.wrappedExpr }; }
    constexpr UpdateStmt operator-=(const wrapped_vector& left, const wrapped_scalar& right) { return { BinaryOperator::MINUS, left.wrappedExpr, right.wrappedExpr }; }
    constexpr UpdateStmt operator*=(const wrapped_vector& left, const wrapped_scalar& right) { return { BinaryOperator::MULTIPLY, left.wrappedExpr, right.wrappedExpr }; }
    constexpr UpdateStmt operator/=(const wrapped_vector& left, const wrapped_scalar& right) { return { BinaryOperator::DIVIDE, left.wrappedExpr, right.wrappedExpr }; }

    constexpr UpdateStmt operator+=(const wrapped_vector& left, const wrapped_vector& right) { return { BinaryOperator::PLUS, left.wrappedExpr, right.wrappedExpr }; }
    constexpr UpdateStmt operator-=(const wrapped_vector& left, const wrapped_vector& right) { return { BinaryOperator::MINUS, left.wrappedExpr, right.wrappedExpr }; }
    constexpr UpdateStmt operator*=(const wrapped_vector& left, const wrapped_vector& right) { return { BinaryOperator::MULTIPLY, left.wrappedExpr, right.wrappedExpr }; }
    constexpr UpdateStmt operator/=(const wrapped_vector& left, const wrapped_vector& right) { return { BinaryOperator::DIVIDE, left.wrappedExpr, right.wrappedExpr }; }
}

}
