#pragma once

#include <string>

namespace Tag {
struct ShaderGen {
    static constexpr char tag[] = "shadergen";
    static constexpr bool verifyEnabled = true;
};
}

namespace shadergen {

enum class ASTNodeKind {
    EXPR,
    VAR_EXPR,
    ELEMENTWISE_BINARY_OPERATOR_EXPR,
    VECTOR_BY_SCALAR_BINARY_OPERATOR_EXPR,
    UNARY_OPERATOR_EXPR,
};

// Types
enum class TypeKind {
    UNKNOWN,
    SCALAR,
    VECTOR,
};

enum class ScalarType {
    BOOL,
    INT,
    FLOAT,
};
static constexpr std::string_view toString(ScalarType type) {
    switch (type) {
    case ScalarType::BOOL:
        return "bool";
    case ScalarType::INT:
        return "int";
    case ScalarType::FLOAT:
        return "float";
    default:
        return "unknown";
    }
}

// Binary operators
enum class BinaryOperatorEnum {
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
    ASSIGNMENT,
};
static constexpr bool isArithmeticOp(BinaryOperatorEnum op) {
    return op == BinaryOperatorEnum::PLUS
        || op == BinaryOperatorEnum::MINUS
        || op == BinaryOperatorEnum::MULTIPLY
        || op == BinaryOperatorEnum::DIVIDE;
}
static constexpr bool isLogicalOp(BinaryOperatorEnum op) {
    return op == BinaryOperatorEnum::AND
        || op == BinaryOperatorEnum::OR;
}
static constexpr bool isComparisonOp(BinaryOperatorEnum op) {
    return op == BinaryOperatorEnum::CMP_LE
        || op == BinaryOperatorEnum::CMP_L
        || op == BinaryOperatorEnum::CMP_EQ
        || op == BinaryOperatorEnum::CMP_GE
        || op == BinaryOperatorEnum::CMP_G
        || op == BinaryOperatorEnum::CMP_NE;
}
static constexpr bool isAssignmentOp(BinaryOperatorEnum op) {
    return op == BinaryOperatorEnum::ASSIGNMENT;
}
static constexpr std::string_view toString(BinaryOperatorEnum op) {
    switch (op) {
    case BinaryOperatorEnum::PLUS:
        return "+";
    case BinaryOperatorEnum::MINUS:
        return "-";
    case BinaryOperatorEnum::MULTIPLY:
        return "*";
    case BinaryOperatorEnum::DIVIDE:
        return "/";
    case BinaryOperatorEnum::CMP_LE:
        return "<=";
    case BinaryOperatorEnum::CMP_L:
        return "<";
    case BinaryOperatorEnum::CMP_EQ:
        return "==";
    case BinaryOperatorEnum::CMP_GE:
        return ">=";
    case BinaryOperatorEnum::CMP_G:
        return ">";
    case BinaryOperatorEnum::CMP_NE:
        return "!=";
    case BinaryOperatorEnum::AND:
        return "&&";
    case BinaryOperatorEnum::OR:
        return "||";
    case BinaryOperatorEnum::ASSIGNMENT:
        return "=";
    default:
        return "unknown";
    }
}

// Unary operators
enum class UnaryOperatorEnum {
    NEGATE,
    NOT,
};
static constexpr bool isArithmeticOp(UnaryOperatorEnum op) { return op == UnaryOperatorEnum::NEGATE; }
static constexpr bool isLogicalOp(UnaryOperatorEnum op) { return op == UnaryOperatorEnum::NOT; }
static constexpr std::string_view toString(UnaryOperatorEnum op) {
    switch (op) {
    case UnaryOperatorEnum::NEGATE:
        return "-";
    case UnaryOperatorEnum::NOT:
        return "!";
    default:
        return "unknown";
    }
}

}
