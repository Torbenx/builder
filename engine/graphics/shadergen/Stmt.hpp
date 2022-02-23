#pragma once

#include "containers/ArrayView.hpp"
#include "util/log.hpp"
#include "util/types.hpp"
#include <array>
#include <string_view>

namespace Tag {
struct ShaderGen {
    static constexpr char tag[] = "shadergen";
    static constexpr bool verifyEnabled = true;
};
}

namespace shadergen {

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
    bool isError;
    constexpr Type(bool isUser, bool isScalar, bool isVector, bool isError)
        : isUser(isUser), isScalar(isScalar), isVector(isVector), isError(isError) { }
};

struct ScalarType;

struct VectorType : Type {
    const ScalarType* scalarType;
    int_t vectorSize;
    bool isArithmetic;
    bool isLogical;

    constexpr VectorType(const ScalarType* scalarType, int_t vectorSize, bool isArithmetic, bool isLogical, bool isScalar = false)
        : Type(false, isScalar, true, false), scalarType(scalarType), vectorSize(vectorSize), isArithmetic(isArithmetic), isLogical(isLogical) { }
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

template<typename T>
struct ScalarTypeFromCXX_v;
template<>
struct ScalarTypeFromCXX_v<bool> { static constexpr const ScalarType* type = &BoolType; };
template<>
struct ScalarTypeFromCXX_v<int> { static constexpr const ScalarType* type = &IntType; };
template<>
struct ScalarTypeFromCXX_v<float> { static constexpr const ScalarType* type = &FloatType; };
template<typename T>
static constexpr const ScalarType* scalarTypeFromCXX = ScalarTypeFromCXX_v<T>::type;

struct UserType : Type {
    ArrayView<const MemberInfo> members;
    int_t fieldCount;
    constexpr UserType()
        : Type(true, false, false, false) { }
};

static constexpr const UserType* asUserType(const Type* type) {
    VERIFY(ShaderGen, type->isUser);
    return static_cast<const UserType*>(type);
}

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
    callback(VectorComponentExpr)        \
    callback(CallExpr)                   \
    callback(ErrorExpr)                  \
    callback(ErrorStmt)

#define ENUMERATE_AST_NODES(callback) \
    ENUMERATE_STMT_KINDS(callback)    \
    callback(ASTListEntry)            \
    callback(VarDecl)                 \
    callback(ErrorNote)
// clang-format on

struct ASTNode { };

// Statement
enum class StmtKind {
#define STMT_KIND(S) S,
    ENUMERATE_STMT_KINDS(STMT_KIND)
#undef STMT_KIND
};

struct Stmt : ASTNode {
    const StmtKind stmtKind;
    constexpr Stmt(StmtKind kind = StmtKind::Stmt)
        : stmtKind(kind) { }
};

template<typename T>
static constexpr T* castStmt(Stmt* stmt) {
    return (T*)stmt;
}
template<typename T>
static constexpr const T* castStmt(const Stmt* stmt) {
    return (const T*)stmt;
}

// AST list
struct ASTListEntry : ASTNode {
    constexpr ASTListEntry(const ASTNode* node)
        : node(node) { }
    const ASTNode* node;
    ASTListEntry* next = nullptr;
};
template<typename T>
struct ASTList {
    bool isLinked = true;
    struct Array {
        T** begin;
        int_t capacity;
    };
    struct Linked {
        ASTListEntry* first = nullptr;
        ASTListEntry* last = nullptr;
    };
    union {
        Linked linked {};
        Array array;
    };
    int_t entryCount = 0;

    constexpr void pushBack(ASTListEntry* newEntry) {
        if (linked.last)
            linked.last->next = newEntry;
        else
            linked.first = newEntry;
        linked.last = newEntry;
        entryCount += 1;
    }
    constexpr void pushBack(T* item) {
        VERIFY(ShaderGen, entryCount < array.capacity);
        array.begin[entryCount] = item;
        entryCount += 1;
    }
    constexpr void clear() {
        isLinked = true;
        std::construct_at(&linked, Linked { nullptr, nullptr });
        entryCount = 0;
    }
    constexpr void initArray(T** begin, int_t cap) {
        isLinked = false;
        std::construct_at(&array, Array { begin, cap });
        entryCount = 0;
    }
    constexpr void setArray(T** begin, int_t size) {
        initArray(begin, size);
        entryCount = size;
    }
    template<typename Callback>
    constexpr void forEachEntry(Callback&& callback) const {
        if (isLinked) {
            ASTListEntry* entry = linked.first;
            while (entry) {
                callback((T*)entry->node);
                entry = entry->next;
            }
        } else {
            for (int_t i = 0; i < entryCount; i++) {
                callback(array.begin[i]);
            }
        }
    };
    template<typename Callback>
    constexpr void consumeEntries(Callback&& callback) {
        if (isLinked) {
            ASTListEntry* entry = linked.first;
            clear();
            while (entry) {
                callback((T*)entry->node);
                entry = entry->next;
            }
        } else {
            T** begin = array.begin;
            int_t count = entryCount;
            clear();
            for (int_t i = 0; i < count; i++) {
                callback(begin[i]);
            }
        }
    }
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

// Errors
static constexpr Type ErrorType = Type(false, false, false, true);
struct ErrorNote : ASTNode {
    std::string_view message;
};
struct ErrorNode {
    std::string_view message;
    const Expr* causingSubExpr = nullptr;
    ASTList<ErrorNote> notes;

    constexpr bool hasError() { return !message.empty(); }
    constexpr void clear() {
        message = {};
        causingSubExpr = nullptr;
        notes.clear();
    }
};
struct ErrorStmt : Stmt, ErrorNode {
    Stmt* subStmt = nullptr;
    constexpr ErrorStmt(const ErrorNode& error)
        : Stmt(StmtKind::ErrorStmt), ErrorNode(error) { }
};
struct ErrorExpr : Expr, ErrorNode {
    Expr* subExpr = nullptr;
    constexpr ErrorExpr(const ErrorNode& error)
        : Expr(StmtKind::ErrorExpr), ErrorNode(error) { }
};

// Variables
struct VarDeclContext;
struct VarDecl : ASTNode {
    std::string_view name;
    const Type* type;
    VarDeclContext* declContext;
    FixedBitset initMask = 0;
    FixedBitset usedMask = 0;
    constexpr VarDecl(std::string_view name, const Type* type, VarDeclContext* declContext)
        : name(name), type(type), declContext(declContext) { }
};
struct VarDeclStmt : Stmt {
    VarDecl* decl;
    constexpr VarDeclStmt(VarDecl* decl)
        : Stmt(StmtKind::VarDeclStmt), decl(decl) { }
};
struct VarDeclContext {
    ASTList<VarDecl> decls;
};
struct VarRefExpr : Expr {
    VarDecl* decl;
    constexpr VarRefExpr(VarDecl* decl)
        : Expr(StmtKind::VarRefExpr), decl(decl) { }
};

// Assignment
struct AssignmentStmt : Stmt {
    const Expr* left;
    const Expr* right;
    constexpr AssignmentStmt(const Expr* left, const Expr* right)
        : Stmt(StmtKind::AssignmentStmt), left(left), right(right) { }
};

template<typename T>
struct wrapped_base;

template<typename S, typename T>
struct wrapped : S, wrapped_base<T> {
    constexpr wrapped(const S& s)
        : S(s), wrapped_base<T> { (S*)this } { }

    constexpr AssignmentStmt operator=(const wrapped<S, T>& expr) {
        return { this, expr.wrappedExpr };
    }
    constexpr AssignmentStmt operator=(const wrapped_base<T>& expr) {
        return { this, expr.wrappedExpr };
    }
};

template<>
struct wrapped_base<void> {
    const Expr* wrappedExpr;
};

// Member access
struct MemberAccessExpr : Expr {
    const Expr* base;
    int_t memberIndex;
    constexpr MemberAccessExpr(const Expr* base, int_t index)
        : Expr(StmtKind::MemberAccessExpr), base(base), memberIndex(index) { }
};

// Vector components
struct VectorComponentExpr : Expr {
    const Expr* base;
    int32_t componentCount;
    int8_t components[4] = {};
    constexpr VectorComponentExpr(const Expr* base, int_t n, const int8_t (&arr)[4])
        : Expr(StmtKind::VectorComponentExpr), base(base), componentCount(n) { std::copy_n(arr, n, components); }
    template<int_t N>
    constexpr VectorComponentExpr(const Expr* base, const int8_t (&arr)[N])
        : Expr(StmtKind::VectorComponentExpr), base(base), componentCount(N) { std::copy_n(arr, N, components); }
};

struct vec_t { };

template<>
struct wrapped_base<vec_t> {
    const Expr* wrappedExpr;

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
    const Expr* subExpr;
    constexpr UnaryOperatorExpr(UnaryOperator op, const Expr* subExpr)
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
    const Expr* left;
    const Expr* right;
    constexpr ElementwiseOperatorExpr(BinaryOperator op, const Expr* left, const Expr* right)
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
    const Expr* vector;
    const Expr* scalar;
    bool isVectorLeft;
    constexpr VectorByScalarOperatorExpr(BinaryOperator op, const Expr* vector, const Expr* scalar, bool isVectorLeft)
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
    template<int_t N>
    constexpr LiteralStorage(const bool (&in)[N])
        : bools() { std::copy_n(in, N, bools); }
    template<int_t N>
    constexpr LiteralStorage(const int (&in)[N])
        : ints() { std::copy_n(in, N, ints); }
    template<int_t N>
    constexpr LiteralStorage(const float (&in)[N])
        : floats() { std::copy_n(in, N, floats); }
};
struct LiteralExpr : Expr {
    LiteralStorage literal;

    constexpr LiteralExpr(const Type* type, LiteralStorage literal)
        : Expr(StmtKind::LiteralExpr), literal(literal) {
        this->type = type;
    }
};

namespace code {
    template<typename T>
    constexpr wrapped<LiteralExpr, void> constant(T p) { return { { scalarTypeFromCXX<T>, { { p } } } }; }
    template<typename T>
    constexpr wrapped<LiteralExpr, vec_t> constant(T p1, T p2) { return { { scalarTypeFromCXX<T>->vectorType(2), { { p1, p2 } } } }; }
    template<typename T>
    constexpr wrapped<LiteralExpr, vec_t> constant(T p1, T p2, T p3) { return { { scalarTypeFromCXX<T>->vectorType(3), { { p1, p2, p3 } } } }; }
    template<typename T>
    constexpr wrapped<LiteralExpr, vec_t> constant(T p1, T p2, T p3, T p4) { return { { scalarTypeFromCXX<T>->vectorType(4), { { p1, p2, p3, p4 } } } }; }
};

// Block statement
struct CompoundStmt : Stmt, ASTList<Stmt> {
    constexpr CompoundStmt()
        : Stmt(StmtKind::CompoundStmt) { }
};

struct BlockStmt : Stmt {
    BlockStmt* parent = nullptr;
    Stmt* body;
    VarDeclContext declContext;
    constexpr BlockStmt(Stmt* body)
        : Stmt(StmtKind::BlockStmt), body(body) { }
};

// Functions
struct FunctionParameter {
    bool isDynamic;
    struct DynamicParameter {
        const ScalarType* scalarType;
    };
    struct FixedParameter {
        const Type* type;
    };
    union {
        DynamicParameter dynamic;
        FixedParameter fixed;
    };
    static constexpr FunctionParameter makeFixed(const Type* type) {
        return { .isDynamic = false, .fixed = { type } };
    }
    static constexpr FunctionParameter makeDynamic(const ScalarType* baseType) {
        return { .isDynamic = true, .dynamic = { baseType } };
    }
};
struct Function {
    bool isGroup;
    std::string_view name;
    constexpr Function(bool isGroup, std::string_view name)
        : isGroup(isGroup), name(name) { }
};
struct FunctionGroup;
struct FunctionInstance : Function {
    const FunctionGroup* group = nullptr;
    ArrayView<const FunctionParameter> parameters;
    FunctionParameter returnType;
    bool dynamicScalarAllowed = false;
    constexpr FunctionInstance(std::string_view name, ArrayView<const FunctionParameter> parameters, FunctionParameter returnType, bool dynamicScalarAllowed = false)
        : Function(false, name), parameters(parameters), returnType(returnType), dynamicScalarAllowed(dynamicScalarAllowed) { }
};
template<int_t N>
struct BuiltinFunctionInstance : FunctionInstance {
    FunctionParameter parms[N];
    constexpr BuiltinFunctionInstance(std::string_view name, const FunctionParameter (&parameters)[N], FunctionParameter returnType, bool dynamicScalarAllowed = false)
        : FunctionInstance(name, parms, returnType, dynamicScalarAllowed) {
        std::copy_n(parameters, N, parms);
    }
};
struct FunctionGroup : Function {
    ArrayView<const FunctionInstance*> instances;
    constexpr FunctionGroup(std::string_view name)
        : Function(true, name) { }
};
static constexpr const FunctionInstance* asFunctionInstance(const Function* func) {
    return (const FunctionInstance*)func;
}
static constexpr const FunctionGroup* asFunctionGroup(const Function* func) {
    return (const FunctionGroup*)func;
}

struct CallExpr : Expr {
    const Function* function;
    ASTList<const Expr> arguments;
    constexpr CallExpr(const Function* function)
        : Expr(StmtKind::CallExpr), function(function) { }
};
template<int_t N>
struct UnresolvedCall : CallExpr {
    const Expr* argumentStorage[N];
    constexpr UnresolvedCall(const Function* function, std::array<const Expr*, N> args)
        : CallExpr(function) {
        std::copy_n(args.data(), N, argumentStorage);
        arguments.setArray(argumentStorage, N);
    }
    constexpr UnresolvedCall(const UnresolvedCall& other)
        : CallExpr(other.function) {
        std::copy_n(other.argumentStorage, N, argumentStorage);
        arguments.setArray(argumentStorage, N);
    }
};

static constexpr BuiltinFunctionInstance funcFloorVector { "floor", { FunctionParameter::makeDynamic(&FloatType) }, FunctionParameter::makeDynamic(&FloatType) };
static constexpr BuiltinFunctionInstance funcFloorScalar { "floor", { FunctionParameter::makeFixed(&FloatType) }, FunctionParameter::makeFixed(&FloatType) };

namespace code {
    constexpr wrapped<UnresolvedCall<1>, vec_t> floor(const wrapped_base<vec_t>& arg) { return { { &funcFloorVector, { arg.wrappedExpr } } }; }
    constexpr wrapped<UnresolvedCall<1>, void> floor(const wrapped_base<void>& arg) { return { { &funcFloorScalar, { arg.wrappedExpr } } }; }
}

}
