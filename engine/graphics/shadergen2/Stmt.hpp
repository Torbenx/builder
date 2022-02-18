#pragma once

#include "util/log.hpp"
#include "util/types.hpp"
#include <string>

namespace Tag {
struct ShaderGen {
    static constexpr char tag[] = "shadergen";
    static constexpr bool verifyEnabled = true;
};
}

namespace shadergen {

// DynamicName
static constexpr int_t MAX_NAME_LENGTH = 22;

struct DynamicName {
    char storage[MAX_NAME_LENGTH + 1] = {};
    int8_t m_length = 0;

    constexpr DynamicName() { }
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

// forward declare AST
struct AST;

// Types
struct Type {
    bool isUser;
    bool isScalar;
    bool isVector;
    constexpr Type(bool isUser = true, bool isScalar = false, bool isVector = false)
        : isUser(isUser), isScalar(isScalar), isVector(isVector) { }
};

struct ScalarType;

struct VectorType : Type {
    const ScalarType* scalarType;
    int_t vectorSize;
    bool isArithmetic;
    bool isLogical;

    constexpr VectorType(const ScalarType* scalarType, int_t vectorSize, bool isArithmetic, bool isLogical, bool isScalar = false)
        : Type(false, isScalar, true), scalarType(scalarType), vectorSize(vectorSize), isArithmetic(isArithmetic), isLogical(isLogical) { }
};

struct ScalarType : VectorType {
    VectorType vectorTypes[3] = {
        { this, 2, isArithmetic, isLogical },
        { this, 3, isArithmetic, isLogical },
        { this, 4, isArithmetic, isLogical }
    };
    std::string_view name;

    constexpr const VectorType* vectorType(int_t size) const {
        if (size == 1)
            return this;
        else
            return &vectorTypes[size - 2];
    }

    constexpr ScalarType(std::string_view name, bool isArithmetic, bool isLogical)
        : VectorType(this, 1, isArithmetic, isLogical, true), name(name) { }
};

static constexpr const VectorType* asVectorType(const Type* type) {
    VERIFY(ShaderGen, type->isVector);
    return static_cast<const VectorType*>(type);
}

static constexpr const ScalarType* asScalarType(const Type* type) {
    VERIFY(ShaderGen, type->isScalar);
    return static_cast<const ScalarType*>(type);
}

static constexpr ScalarType BoolType { "bool", false, true };
static constexpr ScalarType IntType { "int", true, false };
static constexpr ScalarType FloatType { "float", true, false };

// Statements
// clang-format off
#define ENUMERATE_STMT_KINDS(callback)   \
    callback(Stmt)                       \
    callback(Expr)                       \
    callback(VarDeclStmt)                \
    callback(VarRefExpr)                 \
    callback(UnaryOperatorExpr)          \
    callback(MemberAccessExpr)           \
    callback(ElementwiseOperatorExpr)    \
    callback(VectorByScalarOperatorExpr) \
    callback(CompoundStmt)               \
    callback(BlockStmt)                  \
    callback(LiteralExpr)                \
    callback(AssignmentStmt)

#define ENUMERATE_AST_NODES(callback) \
    ENUMERATE_STMT_KINDS(callback)    \
    callback(CompoundStmtEntry)
// clang-format on

enum class StmtKind {
#define STMT_KIND(S) S,
    ENUMERATE_STMT_KINDS(STMT_KIND)
#undef STMT_KIND
};

struct Stmt {
    const StmtKind stmtKind;
    constexpr Stmt(StmtKind kind = StmtKind::Stmt)
        : stmtKind(kind) { }
};

template<typename T>
static constexpr T& castStmt(Stmt& stmt) {
    return (T&)stmt;
}

struct Expr : Stmt {
    const Type* type = nullptr;
    constexpr Expr(StmtKind kind = StmtKind::Expr)
        : Stmt(kind) { }
};

// Variables
struct VarDeclStmt : Stmt {
    DynamicName name;
    const Type* type;
    constexpr VarDeclStmt(DynamicName name, const Type* type)
        : Stmt(StmtKind::VarDeclStmt), name(name), type(type) { }
};
struct VarRefExpr : Expr {
    VarDeclStmt& decl;
    constexpr VarRefExpr(VarDeclStmt& decl)
        : Expr(StmtKind::VarRefExpr), decl(decl) { }
};

// Assignment
struct AssignmentStmt : Stmt {
    const Expr& left;
    const Expr& right;
    constexpr AssignmentStmt(const Expr& left, const Expr& right)
        : Stmt(StmtKind::AssignmentStmt), left(left), right(right) { }
};

// Member access
struct MemberAccessExpr : Expr {
    const Expr& base;
    int_t memberIndex;
    constexpr MemberAccessExpr(const Expr& base, int_t index)
        : Expr(StmtKind::MemberAccessExpr), base(base), memberIndex(index) { }
};

struct vec_t { };

template<typename T>
struct wrapped_base;

template<typename S, typename T>
struct wrapped : S, wrapped_base<T> {
    constexpr wrapped(const S& s)
        : S(s), wrapped_base<T> { (S&)*this } { }

    constexpr AssignmentStmt operator=(const wrapped<S, T>& expr) {
        return { *this, expr };
    }
    constexpr AssignmentStmt operator=(const wrapped_base<T>& expr) {
        return { *this, expr.wrappedExpr };
    }
};

template<>
struct wrapped_base<void> {
    const Expr& wrappedExpr;
};

template<>
struct wrapped_base<vec_t> {
    const Expr& wrappedExpr;
    wrapped<MemberAccessExpr, void> x { { wrappedExpr, 0 } };
    wrapped<MemberAccessExpr, void> y { { wrappedExpr, 1 } };
    wrapped<MemberAccessExpr, void> z { { wrappedExpr, 2 } };
    wrapped<MemberAccessExpr, void> w { { wrappedExpr, 3 } };
};

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
        return "unknown";
    }
}
struct UnaryOperatorExpr : Expr {
    UnaryOperator op;
    const Expr& subExpr;
    constexpr UnaryOperatorExpr(UnaryOperator op, const Expr& subExpr)
        : Expr(StmtKind::UnaryOperatorExpr), op(op), subExpr(subExpr) { }
};
namespace code {
    constexpr wrapped<UnaryOperatorExpr, void> operator-(const wrapped_base<void>& expr) { return { { UnaryOperator::NEGATE, expr.wrappedExpr } }; }
    constexpr wrapped<UnaryOperatorExpr, vec_t> operator-(const wrapped_base<vec_t>& expr) { return { { UnaryOperator::NEGATE, expr.wrappedExpr } }; }
    constexpr wrapped<UnaryOperatorExpr, void> operator!(const wrapped_base<void>& expr) { return { { UnaryOperator::NOT, expr.wrappedExpr } }; }
    constexpr wrapped<UnaryOperatorExpr, vec_t> operator!(const wrapped_base<vec_t>& expr) { return { { UnaryOperator::NOT, expr.wrappedExpr } }; }
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
        return "unknown";
    }
}

// Elementwise operators
struct ElementwiseOperatorExpr : Expr {
    BinaryOperator op;
    const Expr& left;
    const Expr& right;
    constexpr ElementwiseOperatorExpr(BinaryOperator op, const Expr& left, const Expr& right)
        : Expr(StmtKind::ElementwiseOperatorExpr), op(op), left(left), right(right) { }
};
namespace code {
    constexpr wrapped<ElementwiseOperatorExpr, void> operator+(const wrapped_base<void>& left, const wrapped_base<void>& right) { return { { BinaryOperator::PLUS, left.wrappedExpr, right.wrappedExpr } }; }
    constexpr wrapped<ElementwiseOperatorExpr, void> operator-(const wrapped_base<void>& left, const wrapped_base<void>& right) { return { { BinaryOperator::MINUS, left.wrappedExpr, right.wrappedExpr } }; }
    constexpr wrapped<ElementwiseOperatorExpr, void> operator*(const wrapped_base<void>& left, const wrapped_base<void>& right) { return { { BinaryOperator::MULTIPLY, left.wrappedExpr, right.wrappedExpr } }; }
    constexpr wrapped<ElementwiseOperatorExpr, void> operator/(const wrapped_base<void>& left, const wrapped_base<void>& right) { return { { BinaryOperator::DIVIDE, left.wrappedExpr, right.wrappedExpr } }; }

    constexpr wrapped<ElementwiseOperatorExpr, void> operator&&(const wrapped_base<void>& left, const wrapped_base<void>& right) { return { { BinaryOperator::AND, left.wrappedExpr, right.wrappedExpr } }; }
    constexpr wrapped<ElementwiseOperatorExpr, void> operator||(const wrapped_base<void>& left, const wrapped_base<void>& right) { return { { BinaryOperator::OR, left.wrappedExpr, right.wrappedExpr } }; }

    constexpr wrapped<ElementwiseOperatorExpr, void> operator<(const wrapped_base<void>& left, const wrapped_base<void>& right) { return { { BinaryOperator::CMP_L, left.wrappedExpr, right.wrappedExpr } }; }
    constexpr wrapped<ElementwiseOperatorExpr, void> operator<=(const wrapped_base<void>& left, const wrapped_base<void>& right) { return { { BinaryOperator::CMP_LE, left.wrappedExpr, right.wrappedExpr } }; }
    constexpr wrapped<ElementwiseOperatorExpr, void> operator==(const wrapped_base<void>& left, const wrapped_base<void>& right) { return { { BinaryOperator::CMP_EQ, left.wrappedExpr, right.wrappedExpr } }; }
    constexpr wrapped<ElementwiseOperatorExpr, void> operator>=(const wrapped_base<void>& left, const wrapped_base<void>& right) { return { { BinaryOperator::CMP_GE, left.wrappedExpr, right.wrappedExpr } }; }
    constexpr wrapped<ElementwiseOperatorExpr, void> operator>(const wrapped_base<void>& left, const wrapped_base<void>& right) { return { { BinaryOperator::CMP_G, left.wrappedExpr, right.wrappedExpr } }; }
    constexpr wrapped<ElementwiseOperatorExpr, void> operator!=(const wrapped_base<void>& left, const wrapped_base<void>& right) { return { { BinaryOperator::CMP_NE, left.wrappedExpr, right.wrappedExpr } }; }

    constexpr wrapped<ElementwiseOperatorExpr, vec_t> operator+(const wrapped_base<vec_t>& left, const wrapped_base<vec_t>& right) { return { { BinaryOperator::PLUS, left.wrappedExpr, right.wrappedExpr } }; }
    constexpr wrapped<ElementwiseOperatorExpr, vec_t> operator-(const wrapped_base<vec_t>& left, const wrapped_base<vec_t>& right) { return { { BinaryOperator::MINUS, left.wrappedExpr, right.wrappedExpr } }; }
    constexpr wrapped<ElementwiseOperatorExpr, vec_t> operator*(const wrapped_base<vec_t>& left, const wrapped_base<vec_t>& right) { return { { BinaryOperator::MULTIPLY, left.wrappedExpr, right.wrappedExpr } }; }
    constexpr wrapped<ElementwiseOperatorExpr, vec_t> operator/(const wrapped_base<vec_t>& left, const wrapped_base<vec_t>& right) { return { { BinaryOperator::DIVIDE, left.wrappedExpr, right.wrappedExpr } }; }

    constexpr wrapped<ElementwiseOperatorExpr, vec_t> operator&&(const wrapped_base<vec_t>& left, const wrapped_base<vec_t>& right) { return { { BinaryOperator::AND, left.wrappedExpr, right.wrappedExpr } }; }
    constexpr wrapped<ElementwiseOperatorExpr, vec_t> operator||(const wrapped_base<vec_t>& left, const wrapped_base<vec_t>& right) { return { { BinaryOperator::OR, left.wrappedExpr, right.wrappedExpr } }; }

    constexpr wrapped<ElementwiseOperatorExpr, vec_t> operator<(const wrapped_base<vec_t>& left, const wrapped_base<vec_t>& right) { return { { BinaryOperator::CMP_L, left.wrappedExpr, right.wrappedExpr } }; }
    constexpr wrapped<ElementwiseOperatorExpr, vec_t> operator<=(const wrapped_base<vec_t>& left, const wrapped_base<vec_t>& right) { return { { BinaryOperator::CMP_LE, left.wrappedExpr, right.wrappedExpr } }; }
    constexpr wrapped<ElementwiseOperatorExpr, vec_t> operator==(const wrapped_base<vec_t>& left, const wrapped_base<vec_t>& right) { return { { BinaryOperator::CMP_EQ, left.wrappedExpr, right.wrappedExpr } }; }
    constexpr wrapped<ElementwiseOperatorExpr, vec_t> operator>=(const wrapped_base<vec_t>& left, const wrapped_base<vec_t>& right) { return { { BinaryOperator::CMP_GE, left.wrappedExpr, right.wrappedExpr } }; }
    constexpr wrapped<ElementwiseOperatorExpr, vec_t> operator>(const wrapped_base<vec_t>& left, const wrapped_base<vec_t>& right) { return { { BinaryOperator::CMP_G, left.wrappedExpr, right.wrappedExpr } }; }
    constexpr wrapped<ElementwiseOperatorExpr, vec_t> operator!=(const wrapped_base<vec_t>& left, const wrapped_base<vec_t>& right) { return { { BinaryOperator::CMP_NE, left.wrappedExpr, right.wrappedExpr } }; }
}

// Vector by Scalar operators
struct VectorByScalarOperatorExpr : Expr {
    BinaryOperator op;
    const Expr& vector;
    const Expr& scalar;
    bool isVectorLeft;
    constexpr VectorByScalarOperatorExpr(BinaryOperator op, const Expr& vector, const Expr& scalar, bool isVectorLeft)
        : Expr(StmtKind::VectorByScalarOperatorExpr), op(op), vector(vector), scalar(scalar), isVectorLeft(isVectorLeft) { }
};
namespace code {
    constexpr wrapped<VectorByScalarOperatorExpr, vec_t> operator+(const wrapped_base<vec_t>& left, const wrapped_base<void>& right) { return { { BinaryOperator::PLUS, left.wrappedExpr, right.wrappedExpr, true } }; }
    constexpr wrapped<VectorByScalarOperatorExpr, vec_t> operator-(const wrapped_base<vec_t>& left, const wrapped_base<void>& right) { return { { BinaryOperator::MINUS, left.wrappedExpr, right.wrappedExpr, true } }; }
    constexpr wrapped<VectorByScalarOperatorExpr, vec_t> operator*(const wrapped_base<vec_t>& left, const wrapped_base<void>& right) { return { { BinaryOperator::MULTIPLY, left.wrappedExpr, right.wrappedExpr, true } }; }
    constexpr wrapped<VectorByScalarOperatorExpr, vec_t> operator/(const wrapped_base<vec_t>& left, const wrapped_base<void>& right) { return { { BinaryOperator::DIVIDE, left.wrappedExpr, right.wrappedExpr, true } }; }

    constexpr wrapped<VectorByScalarOperatorExpr, vec_t> operator+(const wrapped_base<void>& left, const wrapped_base<vec_t>& right) { return { { BinaryOperator::PLUS, right.wrappedExpr, left.wrappedExpr, false } }; }
    constexpr wrapped<VectorByScalarOperatorExpr, vec_t> operator-(const wrapped_base<void>& left, const wrapped_base<vec_t>& right) { return { { BinaryOperator::MINUS, right.wrappedExpr, left.wrappedExpr, false } }; }
    constexpr wrapped<VectorByScalarOperatorExpr, vec_t> operator*(const wrapped_base<void>& left, const wrapped_base<vec_t>& right) { return { { BinaryOperator::MULTIPLY, right.wrappedExpr, left.wrappedExpr, false } }; }
    constexpr wrapped<VectorByScalarOperatorExpr, vec_t> operator/(const wrapped_base<void>& left, const wrapped_base<vec_t>& right) { return { { BinaryOperator::DIVIDE, right.wrappedExpr, left.wrappedExpr, false } }; }
};

// Literals
union LiteralStorage {
    bool bools[4];
    int ints[4];
    float floats[4];

    constexpr LiteralStorage() { }
};
struct LiteralExpr : Expr {
    LiteralStorage literal;

    constexpr LiteralExpr(LiteralStorage literal)
        : Expr(StmtKind::LiteralExpr), literal(literal) { }
};

// Compound statements
struct CompoundStmtEntry {
    Stmt& stmt;
    CompoundStmtEntry* next = nullptr;
};
struct CompoundStmt : Stmt {
    CompoundStmtEntry* first = nullptr;
    CompoundStmtEntry* last = nullptr;
    int_t childCount = 0;
    constexpr CompoundStmt(StmtKind kind = StmtKind::CompoundStmt)
        : Stmt(kind) { }

    constexpr CompoundStmtEntry& pushBack(AST& ast, Stmt& stmt);

    template<typename Callback>
    constexpr void forEachStmt(Callback&& callback) const {
        const CompoundStmtEntry* entry = first;
        while (entry) {
            callback(entry->stmt);
            entry = entry->next;
        }
    };
};
struct BlockStmt : CompoundStmt {
    BlockStmt* parent = nullptr;
    constexpr BlockStmt()
        : CompoundStmt(StmtKind::BlockStmt) { }
};

}
