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

enum class ValueKind {
    LVALUE,
    RVALUE
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

// DynamicName
static constexpr int_t MAX_NAME_LENGTH = 22;

struct DynamicName {
    char storage[MAX_NAME_LENGTH + 1] = {};
    int8_t m_length = 0;

    constexpr DynamicName() { }
    template<int_t N>
    constexpr DynamicName(fixed_string<N> str)
        : m_length(N) {
        static_assert(str.length() <= MAX_NAME_LENGTH);
        std::copy_n(str.data(), length() + 1, storage);
    }
    constexpr DynamicName(std::string_view view)
        : m_length(view.length()) {
        std::copy_n(view.data(), length() + 1, storage);
    }

    constexpr char* data() {
        return storage;
    }
    constexpr std::string_view view() const { return { data(), (size_t)length() }; }
    constexpr const char* data() const {
        return storage;
    }
    constexpr int_t length() const { return m_length; }
};

// Handles
struct TypeInfo {
    bool scalar = false;
    bool vector = false;
    ScalarType m_scalarType;
    int_t m_vectorSize = 0;
    constexpr bool isBuiltin() const { return true; }
    constexpr bool isVector() const { return vector; }
    constexpr bool isScalar() const { return scalar; }
    constexpr ScalarType scalarType() const { return m_scalarType; }
    constexpr int_t vectorSize() const { return m_vectorSize; }
    // StaticVector<FieldDecl, 20> fields;
    // TypeIdentifier typeId;
};
struct TypeHandle {
    bool builtin = false;
    int_t index = -1;
};

static constexpr int_t BLOCK_SHIFT = 8;
using blockidx_t = size_t;
static constexpr blockidx_t SUB_BLOCK_MASK = ((blockidx_t)1 << BLOCK_SHIFT) - 1;

static constexpr bool isSubBlock(blockidx_t parentIndex, blockidx_t childIndex) {
    int_t parentDepth = (std::numeric_limits<blockidx_t>::digits - std::countl_zero(parentIndex) - 1) / BLOCK_SHIFT;
    int_t childDepth = (std::numeric_limits<blockidx_t>::digits - std::countl_zero(childIndex) - 1) / BLOCK_SHIFT;
    if (parentDepth > childDepth)
        return false;
    blockidx_t relChildIndex = childIndex >> ((childDepth - parentDepth) * BLOCK_SHIFT);
    return relChildIndex == parentIndex;
}

struct VarHandle {
    int_t index = -1;
    blockidx_t block = 0;
};

}
