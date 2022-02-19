#pragma once

#include "containers/ArrayView.hpp"
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

// Fixed bitset
struct FixedBitset {
    size_t mask = 0;
    static constexpr size_t bit(int_t i) { return (size_t)1 << i; }
    static constexpr FixedBitset bitsBelow(int_t n) { return { bit(n) - 1 }; }
    constexpr FixedBitset() = default;
    constexpr FixedBitset(size_t mask)
        : mask(mask) { }

    constexpr bool test(int_t i) const { return mask & bit(i); }
    constexpr void set(int_t i) {
        mask |= bit(i);
    }
    constexpr void clear(int_t i) {
        mask &= bit(i);
    }
    constexpr FixedBitset& operator|=(const FixedBitset& other) {
        mask |= other.mask;
        return *this;
    }
    constexpr FixedBitset& operator&=(const FixedBitset& other) {
        mask &= other.mask;
        return *this;
    }
    constexpr FixedBitset operator~() const {
        return { ~mask };
    }
    constexpr explicit operator bool() const { return mask; }
    constexpr bool operator==(const FixedBitset& other) const {
        return mask == other.mask;
    }
};
constexpr FixedBitset operator|(const FixedBitset& left, const FixedBitset& right) {
    return { left.mask | right.mask };
}
constexpr FixedBitset operator&(const FixedBitset& left, const FixedBitset& right) {
    return { left.mask & right.mask };
}

// forward declare AST
struct AST;

// Types
struct Type;

struct MemberInfo {
    std::string_view name;
    FixedBitset fieldMask;
    const Type* type;
};

struct Type {
    bool isUser;
    bool isScalar;
    bool isVector;
    ArrayView<const MemberInfo> members;
    int_t fieldCount;
    constexpr Type(ArrayView<const MemberInfo> members, int_t fieldCount, bool isUser = true, bool isScalar = false, bool isVector = false)
        : members(members), fieldCount(fieldCount), isUser(isUser), isScalar(isScalar), isVector(isVector) { }
};

struct ScalarType;

struct VectorType : Type {
    const ScalarType* scalarType;
    int_t vectorSize;
    bool isArithmetic;
    bool isLogical;

    constexpr VectorType(const ScalarType* scalarType, int_t vectorSize, bool isArithmetic, bool isLogical, bool isScalar = false)
        : Type({}, 0, false, isScalar, true), scalarType(scalarType), vectorSize(vectorSize), isArithmetic(isArithmetic), isLogical(isLogical) { }
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
    callback(AssignmentStmt)             \
    callback(VectorComponentExpr)

#define ENUMERATE_AST_NODES(callback) \
    ENUMERATE_STMT_KINDS(callback)    \
    callback(StmtList)                \
    callback(StmtListEntry)
// clang-format on

// Statement
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
template<typename T>
static constexpr const T& castStmt(const Stmt& stmt) {
    return (const T&)stmt;
}

// Statement list
struct StmtListEntry {
    Stmt& stmt;
    StmtListEntry* next = nullptr;
};

struct StmtList {
    StmtListEntry* first = nullptr;
    StmtListEntry* last = nullptr;
    int_t stmtCount = 0;

    constexpr void pushBack(AST& ast, Stmt& stmt);

    template<typename Callback>
    constexpr void forEachStmt(Callback&& callback) const {
        const StmtListEntry* entry = first;
        while (entry) {
            callback(entry->stmt);
            entry = entry->next;
        }
    };
};

// Expression
enum class ValueKind {
    UNKNOWN,
    LVALUE,
    RVALUE,
};

struct Expr : Stmt {
    const Type* type = nullptr;
    ValueKind valueKind = ValueKind::UNKNOWN;
    constexpr Expr(StmtKind kind)
        : Stmt(kind) { }
};

// Variables
struct VarDeclContext;
struct VarDeclStmt : Stmt {
    DynamicName name;
    const Type* type;
    VarDeclContext& declContext;
    FixedBitset initMask = 0;
    FixedBitset usedMask = 0;
    constexpr VarDeclStmt(DynamicName name, const Type* type, VarDeclContext& declContext)
        : Stmt(StmtKind::VarDeclStmt), name(name), type(type), declContext(declContext) { }
};
struct VarDeclContext {
    StmtList decls;
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

// Member access
struct MemberAccessExpr : Expr {
    const Expr& base;
    int_t memberIndex;
    constexpr MemberAccessExpr(const Expr& base, int_t index)
        : Expr(StmtKind::MemberAccessExpr), base(base), memberIndex(index) { }
};

// Vector components
struct VectorComponentExpr : Expr {
    const Expr& base;
    int32_t componentCount;
    int8_t components[4] = {};
    constexpr VectorComponentExpr(const Expr& base, int_t n, const int8_t (&arr)[4])
        : Expr(StmtKind::VectorComponentExpr), base(base), componentCount(n) { std::copy_n(arr, n, components); }
    template<int_t N>
    constexpr VectorComponentExpr(const Expr& base, const int8_t (&arr)[N])
        : Expr(StmtKind::VectorComponentExpr), base(base), componentCount(N) { std::copy_n(arr, N, components); }
};

struct vec_t { };

template<>
struct wrapped_base<vec_t> {
    const Expr& wrappedExpr;

    wrapped<VectorComponentExpr, void> x { { wrappedExpr, { 0 } } };
    wrapped<VectorComponentExpr, void> y { { wrappedExpr, { 1 } } };
    wrapped<VectorComponentExpr, void> z { { wrappedExpr, { 2 } } };
    wrapped<VectorComponentExpr, void> w { { wrappedExpr, { 3 } } };

    constexpr wrapped<VectorComponentExpr, vec_t> xx() { return { { wrappedExpr, { 0, 0 } } }; }
    constexpr wrapped<VectorComponentExpr, vec_t> xy() { return { { wrappedExpr, { 0, 1 } } }; }
    constexpr wrapped<VectorComponentExpr, vec_t> xz() { return { { wrappedExpr, { 0, 2 } } }; }
    constexpr wrapped<VectorComponentExpr, vec_t> xw() { return { { wrappedExpr, { 0, 3 } } }; }

    constexpr wrapped<VectorComponentExpr, vec_t> yx() { return { { wrappedExpr, { 1, 0 } } }; }
    constexpr wrapped<VectorComponentExpr, vec_t> yy() { return { { wrappedExpr, { 1, 1 } } }; }
    constexpr wrapped<VectorComponentExpr, vec_t> yz() { return { { wrappedExpr, { 1, 2 } } }; }
    constexpr wrapped<VectorComponentExpr, vec_t> yw() { return { { wrappedExpr, { 1, 3 } } }; }

    constexpr wrapped<VectorComponentExpr, vec_t> zx() { return { { wrappedExpr, { 2, 0 } } }; }
    constexpr wrapped<VectorComponentExpr, vec_t> zy() { return { { wrappedExpr, { 2, 1 } } }; }
    constexpr wrapped<VectorComponentExpr, vec_t> zz() { return { { wrappedExpr, { 2, 2 } } }; }
    constexpr wrapped<VectorComponentExpr, vec_t> zw() { return { { wrappedExpr, { 2, 3 } } }; }

    constexpr wrapped<VectorComponentExpr, vec_t> wx() { return { { wrappedExpr, { 3, 0 } } }; }
    constexpr wrapped<VectorComponentExpr, vec_t> wy() { return { { wrappedExpr, { 3, 1 } } }; }
    constexpr wrapped<VectorComponentExpr, vec_t> wz() { return { { wrappedExpr, { 3, 2 } } }; }
    constexpr wrapped<VectorComponentExpr, vec_t> ww() { return { { wrappedExpr, { 3, 3 } } }; }

    constexpr wrapped<VectorComponentExpr, vec_t> xxx() { return { { wrappedExpr, { 0, 0, 0 } } }; }
    constexpr wrapped<VectorComponentExpr, vec_t> xxy() { return { { wrappedExpr, { 0, 0, 1 } } }; }
    constexpr wrapped<VectorComponentExpr, vec_t> xxz() { return { { wrappedExpr, { 0, 0, 2 } } }; }
    constexpr wrapped<VectorComponentExpr, vec_t> xxw() { return { { wrappedExpr, { 0, 0, 3 } } }; }

    constexpr wrapped<VectorComponentExpr, vec_t> xyx() { return { { wrappedExpr, { 0, 1, 0 } } }; }
    constexpr wrapped<VectorComponentExpr, vec_t> xyy() { return { { wrappedExpr, { 0, 1, 1 } } }; }
    constexpr wrapped<VectorComponentExpr, vec_t> xyz() { return { { wrappedExpr, { 0, 1, 2 } } }; }
    constexpr wrapped<VectorComponentExpr, vec_t> xyw() { return { { wrappedExpr, { 0, 1, 3 } } }; }

    constexpr wrapped<VectorComponentExpr, vec_t> xzx() { return { { wrappedExpr, { 0, 2, 0 } } }; }
    constexpr wrapped<VectorComponentExpr, vec_t> xzy() { return { { wrappedExpr, { 0, 2, 1 } } }; }
    constexpr wrapped<VectorComponentExpr, vec_t> xzz() { return { { wrappedExpr, { 0, 2, 2 } } }; }
    constexpr wrapped<VectorComponentExpr, vec_t> xzw() { return { { wrappedExpr, { 0, 2, 3 } } }; }

    constexpr wrapped<VectorComponentExpr, vec_t> xwx() { return { { wrappedExpr, { 0, 3, 0 } } }; }
    constexpr wrapped<VectorComponentExpr, vec_t> xwy() { return { { wrappedExpr, { 0, 3, 1 } } }; }
    constexpr wrapped<VectorComponentExpr, vec_t> xwz() { return { { wrappedExpr, { 0, 3, 2 } } }; }
    constexpr wrapped<VectorComponentExpr, vec_t> xww() { return { { wrappedExpr, { 0, 3, 3 } } }; }

    constexpr wrapped<VectorComponentExpr, vec_t> yxx() { return { { wrappedExpr, { 1, 0, 0 } } }; }
    constexpr wrapped<VectorComponentExpr, vec_t> yxy() { return { { wrappedExpr, { 1, 0, 1 } } }; }
    constexpr wrapped<VectorComponentExpr, vec_t> yxz() { return { { wrappedExpr, { 1, 0, 2 } } }; }
    constexpr wrapped<VectorComponentExpr, vec_t> yxw() { return { { wrappedExpr, { 1, 0, 3 } } }; }

    constexpr wrapped<VectorComponentExpr, vec_t> yyx() { return { { wrappedExpr, { 1, 1, 0 } } }; }
    constexpr wrapped<VectorComponentExpr, vec_t> yyy() { return { { wrappedExpr, { 1, 1, 1 } } }; }
    constexpr wrapped<VectorComponentExpr, vec_t> yyz() { return { { wrappedExpr, { 1, 1, 2 } } }; }
    constexpr wrapped<VectorComponentExpr, vec_t> yyw() { return { { wrappedExpr, { 1, 1, 3 } } }; }

    constexpr wrapped<VectorComponentExpr, vec_t> yzx() { return { { wrappedExpr, { 1, 2, 0 } } }; }
    constexpr wrapped<VectorComponentExpr, vec_t> yzy() { return { { wrappedExpr, { 1, 2, 1 } } }; }
    constexpr wrapped<VectorComponentExpr, vec_t> yzz() { return { { wrappedExpr, { 1, 2, 2 } } }; }
    constexpr wrapped<VectorComponentExpr, vec_t> yzw() { return { { wrappedExpr, { 1, 2, 3 } } }; }

    constexpr wrapped<VectorComponentExpr, vec_t> ywx() { return { { wrappedExpr, { 1, 3, 0 } } }; }
    constexpr wrapped<VectorComponentExpr, vec_t> ywy() { return { { wrappedExpr, { 1, 3, 1 } } }; }
    constexpr wrapped<VectorComponentExpr, vec_t> ywz() { return { { wrappedExpr, { 1, 3, 2 } } }; }
    constexpr wrapped<VectorComponentExpr, vec_t> yww() { return { { wrappedExpr, { 1, 3, 3 } } }; }

    constexpr wrapped<VectorComponentExpr, vec_t> zxx() { return { { wrappedExpr, { 2, 0, 0 } } }; }
    constexpr wrapped<VectorComponentExpr, vec_t> zxy() { return { { wrappedExpr, { 2, 0, 1 } } }; }
    constexpr wrapped<VectorComponentExpr, vec_t> zxz() { return { { wrappedExpr, { 2, 0, 2 } } }; }
    constexpr wrapped<VectorComponentExpr, vec_t> zxw() { return { { wrappedExpr, { 2, 0, 3 } } }; }

    constexpr wrapped<VectorComponentExpr, vec_t> zyx() { return { { wrappedExpr, { 2, 1, 0 } } }; }
    constexpr wrapped<VectorComponentExpr, vec_t> zyy() { return { { wrappedExpr, { 2, 1, 1 } } }; }
    constexpr wrapped<VectorComponentExpr, vec_t> zyz() { return { { wrappedExpr, { 2, 1, 2 } } }; }
    constexpr wrapped<VectorComponentExpr, vec_t> zyw() { return { { wrappedExpr, { 2, 1, 3 } } }; }

    constexpr wrapped<VectorComponentExpr, vec_t> zzx() { return { { wrappedExpr, { 2, 2, 0 } } }; }
    constexpr wrapped<VectorComponentExpr, vec_t> zzy() { return { { wrappedExpr, { 2, 2, 1 } } }; }
    constexpr wrapped<VectorComponentExpr, vec_t> zzz() { return { { wrappedExpr, { 2, 2, 2 } } }; }
    constexpr wrapped<VectorComponentExpr, vec_t> zzw() { return { { wrappedExpr, { 2, 2, 3 } } }; }

    constexpr wrapped<VectorComponentExpr, vec_t> zwx() { return { { wrappedExpr, { 2, 3, 0 } } }; }
    constexpr wrapped<VectorComponentExpr, vec_t> zwy() { return { { wrappedExpr, { 2, 3, 1 } } }; }
    constexpr wrapped<VectorComponentExpr, vec_t> zwz() { return { { wrappedExpr, { 2, 3, 2 } } }; }
    constexpr wrapped<VectorComponentExpr, vec_t> zww() { return { { wrappedExpr, { 2, 3, 3 } } }; }

    constexpr wrapped<VectorComponentExpr, vec_t> wxx() { return { { wrappedExpr, { 3, 0, 0 } } }; }
    constexpr wrapped<VectorComponentExpr, vec_t> wxy() { return { { wrappedExpr, { 3, 0, 1 } } }; }
    constexpr wrapped<VectorComponentExpr, vec_t> wxz() { return { { wrappedExpr, { 3, 0, 2 } } }; }
    constexpr wrapped<VectorComponentExpr, vec_t> wxw() { return { { wrappedExpr, { 3, 0, 3 } } }; }

    constexpr wrapped<VectorComponentExpr, vec_t> wyx() { return { { wrappedExpr, { 3, 1, 0 } } }; }
    constexpr wrapped<VectorComponentExpr, vec_t> wyy() { return { { wrappedExpr, { 3, 1, 1 } } }; }
    constexpr wrapped<VectorComponentExpr, vec_t> wyz() { return { { wrappedExpr, { 3, 1, 2 } } }; }
    constexpr wrapped<VectorComponentExpr, vec_t> wyw() { return { { wrappedExpr, { 3, 1, 3 } } }; }

    constexpr wrapped<VectorComponentExpr, vec_t> wzx() { return { { wrappedExpr, { 3, 2, 0 } } }; }
    constexpr wrapped<VectorComponentExpr, vec_t> wzy() { return { { wrappedExpr, { 3, 2, 1 } } }; }
    constexpr wrapped<VectorComponentExpr, vec_t> wzz() { return { { wrappedExpr, { 3, 2, 2 } } }; }
    constexpr wrapped<VectorComponentExpr, vec_t> wzw() { return { { wrappedExpr, { 3, 2, 3 } } }; }

    constexpr wrapped<VectorComponentExpr, vec_t> wwx() { return { { wrappedExpr, { 3, 3, 0 } } }; }
    constexpr wrapped<VectorComponentExpr, vec_t> wwy() { return { { wrappedExpr, { 3, 3, 1 } } }; }
    constexpr wrapped<VectorComponentExpr, vec_t> wwz() { return { { wrappedExpr, { 3, 3, 2 } } }; }
    constexpr wrapped<VectorComponentExpr, vec_t> www() { return { { wrappedExpr, { 3, 3, 3 } } }; }
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

struct CompoundStmt : Stmt, StmtList {
    constexpr CompoundStmt(StmtKind kind = StmtKind::CompoundStmt)
        : Stmt(kind) { }
};

// Block statement
struct BlockStmt : CompoundStmt {
    BlockStmt* parent = nullptr;
    VarDeclContext declContext;
    constexpr BlockStmt()
        : CompoundStmt(StmtKind::BlockStmt) { }
};

}
