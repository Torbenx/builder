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
constexpr FixedBitset operator<<(const FixedBitset& bs, int_t shift) {
    return { bs.mask << shift };
}

// Types
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

struct MemberInfo {
    std::string_view name;
    const Type* type;
    int_t offset;
    int_t leafFieldOffset;
};

struct UserType : Type {
    std::string_view name;
    ArrayView<const MemberInfo> members;
    int_t leafFieldCount;
    constexpr UserType(std::string_view name, ArrayView<const MemberInfo> members)
        : Type(true, false, false, false), name(name), members(members) { }
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
    callback(ErrorStmt)                  \
    callback(CompositionExpr)            \
    callback(IfStmt)                     \
    callback(UpdateStmt)

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
constexpr std::string_view toShortSting(ValueKind valueKind) {
    switch (valueKind) {
    case ValueKind::LVALUE:
        return "l";
    case ValueKind::RVALUE:
        return "r";
    default:
        return "?";
    }
}

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

struct wrapped_untyped {
    const Expr* wrappedExpr;
};
struct wrapped_scalar : wrapped_untyped {
    template<typename E>
    struct impl;
};
template<typename E>
struct wrapped_scalar::impl : E, wrapped_scalar {
    constexpr impl(const E& e)
        : E(e), wrapped_scalar { (Expr*)this } { }

    constexpr AssignmentStmt operator=(const wrapped_scalar::impl<E>& right) {
        return { this, right.wrappedExpr };
    }
    constexpr AssignmentStmt operator=(const wrapped_scalar& right) {
        return { this, right.wrappedExpr };
    }
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

struct wrapped_vector : wrapped_untyped {
    template<typename E>
    struct impl;

    wrapped_scalar::impl<VectorComponentExpr> x { { wrappedExpr, { 0 } } };
    wrapped_scalar::impl<VectorComponentExpr> y { { wrappedExpr, { 1 } } };
    wrapped_scalar::impl<VectorComponentExpr> z { { wrappedExpr, { 2 } } };
    wrapped_scalar::impl<VectorComponentExpr> w { { wrappedExpr, { 3 } } };

    constexpr wrapped_vector::impl<VectorComponentExpr> xx();
    constexpr wrapped_vector::impl<VectorComponentExpr> xy();
    constexpr wrapped_vector::impl<VectorComponentExpr> xz();
    constexpr wrapped_vector::impl<VectorComponentExpr> xw();

    constexpr wrapped_vector::impl<VectorComponentExpr> yx();
    constexpr wrapped_vector::impl<VectorComponentExpr> yy();
    constexpr wrapped_vector::impl<VectorComponentExpr> yz();
    constexpr wrapped_vector::impl<VectorComponentExpr> yw();

    constexpr wrapped_vector::impl<VectorComponentExpr> zx();
    constexpr wrapped_vector::impl<VectorComponentExpr> zy();
    constexpr wrapped_vector::impl<VectorComponentExpr> zz();
    constexpr wrapped_vector::impl<VectorComponentExpr> zw();

    constexpr wrapped_vector::impl<VectorComponentExpr> wx();
    constexpr wrapped_vector::impl<VectorComponentExpr> wy();
    constexpr wrapped_vector::impl<VectorComponentExpr> wz();
    constexpr wrapped_vector::impl<VectorComponentExpr> ww();

    constexpr wrapped_vector::impl<VectorComponentExpr> xxx();
    constexpr wrapped_vector::impl<VectorComponentExpr> xxy();
    constexpr wrapped_vector::impl<VectorComponentExpr> xxz();
    constexpr wrapped_vector::impl<VectorComponentExpr> xxw();

    constexpr wrapped_vector::impl<VectorComponentExpr> xyx();
    constexpr wrapped_vector::impl<VectorComponentExpr> xyy();
    constexpr wrapped_vector::impl<VectorComponentExpr> xyz();
    constexpr wrapped_vector::impl<VectorComponentExpr> xyw();

    constexpr wrapped_vector::impl<VectorComponentExpr> xzx();
    constexpr wrapped_vector::impl<VectorComponentExpr> xzy();
    constexpr wrapped_vector::impl<VectorComponentExpr> xzz();
    constexpr wrapped_vector::impl<VectorComponentExpr> xzw();

    constexpr wrapped_vector::impl<VectorComponentExpr> xwx();
    constexpr wrapped_vector::impl<VectorComponentExpr> xwy();
    constexpr wrapped_vector::impl<VectorComponentExpr> xwz();
    constexpr wrapped_vector::impl<VectorComponentExpr> xww();

    constexpr wrapped_vector::impl<VectorComponentExpr> yxx();
    constexpr wrapped_vector::impl<VectorComponentExpr> yxy();
    constexpr wrapped_vector::impl<VectorComponentExpr> yxz();
    constexpr wrapped_vector::impl<VectorComponentExpr> yxw();

    constexpr wrapped_vector::impl<VectorComponentExpr> yyx();
    constexpr wrapped_vector::impl<VectorComponentExpr> yyy();
    constexpr wrapped_vector::impl<VectorComponentExpr> yyz();
    constexpr wrapped_vector::impl<VectorComponentExpr> yyw();

    constexpr wrapped_vector::impl<VectorComponentExpr> yzx();
    constexpr wrapped_vector::impl<VectorComponentExpr> yzy();
    constexpr wrapped_vector::impl<VectorComponentExpr> yzz();
    constexpr wrapped_vector::impl<VectorComponentExpr> yzw();

    constexpr wrapped_vector::impl<VectorComponentExpr> ywx();
    constexpr wrapped_vector::impl<VectorComponentExpr> ywy();
    constexpr wrapped_vector::impl<VectorComponentExpr> ywz();
    constexpr wrapped_vector::impl<VectorComponentExpr> yww();

    constexpr wrapped_vector::impl<VectorComponentExpr> zxx();
    constexpr wrapped_vector::impl<VectorComponentExpr> zxy();
    constexpr wrapped_vector::impl<VectorComponentExpr> zxz();
    constexpr wrapped_vector::impl<VectorComponentExpr> zxw();

    constexpr wrapped_vector::impl<VectorComponentExpr> zyx();
    constexpr wrapped_vector::impl<VectorComponentExpr> zyy();
    constexpr wrapped_vector::impl<VectorComponentExpr> zyz();
    constexpr wrapped_vector::impl<VectorComponentExpr> zyw();

    constexpr wrapped_vector::impl<VectorComponentExpr> zzx();
    constexpr wrapped_vector::impl<VectorComponentExpr> zzy();
    constexpr wrapped_vector::impl<VectorComponentExpr> zzz();
    constexpr wrapped_vector::impl<VectorComponentExpr> zzw();

    constexpr wrapped_vector::impl<VectorComponentExpr> zwx();
    constexpr wrapped_vector::impl<VectorComponentExpr> zwy();
    constexpr wrapped_vector::impl<VectorComponentExpr> zwz();
    constexpr wrapped_vector::impl<VectorComponentExpr> zww();

    constexpr wrapped_vector::impl<VectorComponentExpr> wxx();
    constexpr wrapped_vector::impl<VectorComponentExpr> wxy();
    constexpr wrapped_vector::impl<VectorComponentExpr> wxz();
    constexpr wrapped_vector::impl<VectorComponentExpr> wxw();

    constexpr wrapped_vector::impl<VectorComponentExpr> wyx();
    constexpr wrapped_vector::impl<VectorComponentExpr> wyy();
    constexpr wrapped_vector::impl<VectorComponentExpr> wyz();
    constexpr wrapped_vector::impl<VectorComponentExpr> wyw();

    constexpr wrapped_vector::impl<VectorComponentExpr> wzx();
    constexpr wrapped_vector::impl<VectorComponentExpr> wzy();
    constexpr wrapped_vector::impl<VectorComponentExpr> wzz();
    constexpr wrapped_vector::impl<VectorComponentExpr> wzw();

    constexpr wrapped_vector::impl<VectorComponentExpr> wwx();
    constexpr wrapped_vector::impl<VectorComponentExpr> wwy();
    constexpr wrapped_vector::impl<VectorComponentExpr> wwz();
    constexpr wrapped_vector::impl<VectorComponentExpr> www();
};
template<typename E>
struct wrapped_vector::impl : E, wrapped_vector {
    constexpr impl(const E& e)
        : E(e), wrapped_vector { (E*)this } { }

    constexpr AssignmentStmt operator=(const impl<E>& right) {
        return { this, right.wrappedExpr };
    }
    constexpr AssignmentStmt operator=(const wrapped_vector& right) {
        return { this, right.wrappedExpr };
    }
};

constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::xx() { return { { wrappedExpr, { 0, 0 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::xy() { return { { wrappedExpr, { 0, 1 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::xz() { return { { wrappedExpr, { 0, 2 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::xw() { return { { wrappedExpr, { 0, 3 } } }; }

constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::yx() { return { { wrappedExpr, { 1, 0 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::yy() { return { { wrappedExpr, { 1, 1 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::yz() { return { { wrappedExpr, { 1, 2 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::yw() { return { { wrappedExpr, { 1, 3 } } }; }

constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::zx() { return { { wrappedExpr, { 2, 0 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::zy() { return { { wrappedExpr, { 2, 1 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::zz() { return { { wrappedExpr, { 2, 2 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::zw() { return { { wrappedExpr, { 2, 3 } } }; }

constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::wx() { return { { wrappedExpr, { 3, 0 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::wy() { return { { wrappedExpr, { 3, 1 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::wz() { return { { wrappedExpr, { 3, 2 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::ww() { return { { wrappedExpr, { 3, 3 } } }; }

constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::xxx() { return { { wrappedExpr, { 0, 0, 0 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::xxy() { return { { wrappedExpr, { 0, 0, 1 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::xxz() { return { { wrappedExpr, { 0, 0, 2 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::xxw() { return { { wrappedExpr, { 0, 0, 3 } } }; }

constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::xyx() { return { { wrappedExpr, { 0, 1, 0 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::xyy() { return { { wrappedExpr, { 0, 1, 1 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::xyz() { return { { wrappedExpr, { 0, 1, 2 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::xyw() { return { { wrappedExpr, { 0, 1, 3 } } }; }

constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::xzx() { return { { wrappedExpr, { 0, 2, 0 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::xzy() { return { { wrappedExpr, { 0, 2, 1 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::xzz() { return { { wrappedExpr, { 0, 2, 2 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::xzw() { return { { wrappedExpr, { 0, 2, 3 } } }; }

constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::xwx() { return { { wrappedExpr, { 0, 3, 0 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::xwy() { return { { wrappedExpr, { 0, 3, 1 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::xwz() { return { { wrappedExpr, { 0, 3, 2 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::xww() { return { { wrappedExpr, { 0, 3, 3 } } }; }

constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::yxx() { return { { wrappedExpr, { 1, 0, 0 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::yxy() { return { { wrappedExpr, { 1, 0, 1 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::yxz() { return { { wrappedExpr, { 1, 0, 2 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::yxw() { return { { wrappedExpr, { 1, 0, 3 } } }; }

constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::yyx() { return { { wrappedExpr, { 1, 1, 0 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::yyy() { return { { wrappedExpr, { 1, 1, 1 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::yyz() { return { { wrappedExpr, { 1, 1, 2 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::yyw() { return { { wrappedExpr, { 1, 1, 3 } } }; }

constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::yzx() { return { { wrappedExpr, { 1, 2, 0 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::yzy() { return { { wrappedExpr, { 1, 2, 1 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::yzz() { return { { wrappedExpr, { 1, 2, 2 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::yzw() { return { { wrappedExpr, { 1, 2, 3 } } }; }

constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::ywx() { return { { wrappedExpr, { 1, 3, 0 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::ywy() { return { { wrappedExpr, { 1, 3, 1 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::ywz() { return { { wrappedExpr, { 1, 3, 2 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::yww() { return { { wrappedExpr, { 1, 3, 3 } } }; }

constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::zxx() { return { { wrappedExpr, { 2, 0, 0 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::zxy() { return { { wrappedExpr, { 2, 0, 1 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::zxz() { return { { wrappedExpr, { 2, 0, 2 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::zxw() { return { { wrappedExpr, { 2, 0, 3 } } }; }

constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::zyx() { return { { wrappedExpr, { 2, 1, 0 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::zyy() { return { { wrappedExpr, { 2, 1, 1 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::zyz() { return { { wrappedExpr, { 2, 1, 2 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::zyw() { return { { wrappedExpr, { 2, 1, 3 } } }; }

constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::zzx() { return { { wrappedExpr, { 2, 2, 0 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::zzy() { return { { wrappedExpr, { 2, 2, 1 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::zzz() { return { { wrappedExpr, { 2, 2, 2 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::zzw() { return { { wrappedExpr, { 2, 2, 3 } } }; }

constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::zwx() { return { { wrappedExpr, { 2, 3, 0 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::zwy() { return { { wrappedExpr, { 2, 3, 1 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::zwz() { return { { wrappedExpr, { 2, 3, 2 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::zww() { return { { wrappedExpr, { 2, 3, 3 } } }; }

constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::wxx() { return { { wrappedExpr, { 3, 0, 0 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::wxy() { return { { wrappedExpr, { 3, 0, 1 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::wxz() { return { { wrappedExpr, { 3, 0, 2 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::wxw() { return { { wrappedExpr, { 3, 0, 3 } } }; }

constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::wyx() { return { { wrappedExpr, { 3, 1, 0 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::wyy() { return { { wrappedExpr, { 3, 1, 1 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::wyz() { return { { wrappedExpr, { 3, 1, 2 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::wyw() { return { { wrappedExpr, { 3, 1, 3 } } }; }

constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::wzx() { return { { wrappedExpr, { 3, 2, 0 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::wzy() { return { { wrappedExpr, { 3, 2, 1 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::wzz() { return { { wrappedExpr, { 3, 2, 2 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::wzw() { return { { wrappedExpr, { 3, 2, 3 } } }; }

constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::wwx() { return { { wrappedExpr, { 3, 3, 0 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::wwy() { return { { wrappedExpr, { 3, 3, 1 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::wwz() { return { { wrappedExpr, { 3, 3, 2 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::www() { return { { wrappedExpr, { 3, 3, 3 } } }; }

// Member access
struct MemberAccessExpr : Expr {
    const Expr* base;
    int_t memberIndex;
    constexpr MemberAccessExpr(const Expr* base, int_t index)
        : Expr(StmtKind::MemberAccessExpr), base(base), memberIndex(index) { }

    constexpr MemberInfo member() const { return asUserType(base->type)->members[memberIndex]; }
};

struct FieldInfo {
    std::string_view name;
    const Type* type = nullptr;
    size_t alignment = 0;
    int_t size = -1;
};

template<typename T>
static constexpr const Type* typeFromCXX = &T::SHADERGEN_TYPE;
template<>
constexpr const Type* typeFromCXX<bool> = &BoolType;
template<>
constexpr const Type* typeFromCXX<int> = &IntType;
template<>
constexpr const Type* typeFromCXX<float> = &FloatType;

template<typename T>
static constexpr FieldInfo makeFieldInfo(std::string_view name) { return { name, typeFromCXX<T>, alignof(T), sizeof(T) }; }

static constexpr int_t getLeafFieldCount(const Type* type) {
    if (type->isVector) {
        return asVectorType(type)->vectorSize;
    } else if (type->isUser) {
        return asUserType(type)->leafFieldCount;
    } else {
        VERIFY_NOT_REACHED(ShaderGen);
        return -1;
    }
}
struct StructLayoutHelper {
    size_t prevEnd = 0;
    constexpr size_t addField(size_t alignment, size_t size) {
        size_t offset = alignmentCeil(alignment, prevEnd);
        prevEnd = offset + size;
        return offset;
    }
};
struct LeafFieldHelper {
    int_t leafFieldCount = 0;
    constexpr int_t addField(const Type* type) {
        int_t offset = leafFieldCount;
        leafFieldCount += getLeafFieldCount(type);
        return offset;
    }
};
template<int_t N>
struct UserTypeImpl : UserType {
    MemberInfo memberInfos[N];
    constexpr UserTypeImpl(std::string_view name)
        : UserType(name, memberInfos) { }
    constexpr UserTypeImpl(const UserTypeImpl& other)
        : UserType(other) {
        members = memberInfos;
        std::copy_n(other.memberInfos, N, memberInfos);
    }
};
static constexpr MemberInfo makeMemberInfo(FieldInfo info, StructLayoutHelper& layout, LeafFieldHelper& leafFields) {
    return { info.name, info.type, (int_t)layout.addField(info.alignment, info.size), leafFields.addField(info.type) };
}
template<typename T, int_t... I>
static constexpr int_t makeUserType_helper(UserTypeImpl<sizeof...(I)>& out, std::integer_sequence<int_t, I...>) {
    StructLayoutHelper layout;
    LeafFieldHelper leafFields;
    ((out.memberInfos[I] = makeMemberInfo(T::template _gen_field_info_<I>, layout, leafFields)), ...);
    return leafFields.leafFieldCount;
}
template<typename T, int_t N>
static constexpr UserTypeImpl<N> makeUserType() {
    UserTypeImpl<N> ret { T::_gen_struct_name_ };
    ret.leafFieldCount = makeUserType_helper<T>(ret, std::make_integer_sequence<int_t, N>());
    return ret;
}

struct wrapped_user_untyped {
    const Expr* wrappedExpr;
};

#define BEGIN_GEN_STRUCT(name)                                           \
    template<int_t fieldIndex>                                           \
    struct _gen_fields_;                                                 \
    template<>                                                           \
    struct _gen_fields_<-1> : wrapped_user_untyped { };                  \
    using _gen_base_alias_ = name;                                       \
    template<int_t fieldIndex>                                           \
    static constexpr ::shadergen::FieldInfo _gen_field_info_;            \
    static constexpr int_t _gen_field_counter_offset_ = __COUNTER__ + 1; \
    static constexpr std::string_view _gen_struct_name_ = #name;

#define GEN_FIELD_HELPER(name, index)                                                                          \
    name;                                                                                                      \
    template<>                                                                                                 \
    struct _gen_fields_<index> : _gen_fields_<index - 1> {                                                     \
        get_wrapper<decltype(_gen_base_alias_::name)>::impl<MemberAccessExpr> name { { wrappedExpr, index } }; \
    };                                                                                                         \
    template<>                                                                                                 \
    static constexpr ::shadergen::FieldInfo _gen_field_info_<index> = ::shadergen::makeFieldInfo<decltype(_gen_base_alias_::name)>(#name);

#define GEN_FIELD(name) GEN_FIELD_HELPER(name, __COUNTER__ - _gen_field_counter_offset_);

#define END_GEN_STRUCT_HELPER(index)                    \
    using _gen_fields_final_ = _gen_fields_<index - 1>; \
    static constexpr ::shadergen::UserTypeImpl SHADERGEN_TYPE = ::shadergen::makeUserType<_gen_base_alias_, index>();

#define END_GEN_STRUCT() END_GEN_STRUCT_HELPER(__COUNTER__ - _gen_field_counter_offset_);

template<typename T>
struct wrapped_user : T::_gen_fields_final_ {
    template<typename E>
    struct impl;
};
template<typename T>
template<typename E>
struct wrapped_user<T>::impl : E, wrapped_user<T> {
    constexpr impl(const E& e)
        : E(e), wrapped_user<T> { (Expr*)this } { }
    constexpr AssignmentStmt operator=(const impl& right) {
        return { this, right.wrappedExpr };
    }
    constexpr AssignmentStmt operator=(const wrapped_user<T>& right) {
        return { this, right.wrappedExpr };
    }
};

template<typename T>
using get_wrapper = std::conditional_t<typeFromCXX<T>->isScalar, wrapped_scalar, std::conditional_t<typeFromCXX<T>->isVector, wrapped_vector, wrapped_user<T>>>;

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
    constexpr wrapped_scalar::impl<LiteralExpr> constant(T p) { return { { asScalarType(typeFromCXX<T>), { { p } } } }; }
    template<typename T>
    constexpr wrapped_vector::impl<LiteralExpr> constant(T p1, T p2) { return { { asScalarType(typeFromCXX<T>)->vectorType(2), { { p1, p2 } } } }; }
    template<typename T>
    constexpr wrapped_vector::impl<LiteralExpr> constant(T p1, T p2, T p3) { return { { asScalarType(typeFromCXX<T>)->vectorType(3), { { p1, p2, p3 } } } }; }
    template<typename T>
    constexpr wrapped_vector::impl<LiteralExpr> constant(T p1, T p2, T p3, T p4) { return { { asScalarType(typeFromCXX<T>)->vectorType(4), { { p1, p2, p3, p4 } } } }; }
}

// Control
struct CompoundStmt : Stmt, ASTList<Stmt> {
    constexpr CompoundStmt()
        : Stmt(StmtKind::CompoundStmt) { }
};

struct BlockStmt : Stmt {
    BlockStmt* parent = nullptr;
    CompoundStmt* body;
    VarDeclContext declContext;
    constexpr BlockStmt(CompoundStmt* body)
        : Stmt(StmtKind::BlockStmt), body(body) { }
};

struct IfStmt : Stmt {
    const Expr* condition;
    Stmt* whenTrue = nullptr;
    Stmt* whenFalse = nullptr;
    constexpr IfStmt(const Expr* condition)
        : Stmt(StmtKind::IfStmt), condition(condition) { }
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
    constexpr BuiltinFunctionInstance(std::string_view name, const FunctionParameter (&parameters)[N], FunctionParameter returnType, bool dynamicScalarAllowed = true)
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

static constexpr BuiltinFunctionInstance floorFunc { "floor", { FunctionParameter::makeDynamic(&FloatType) }, FunctionParameter::makeDynamic(&FloatType) };
static constexpr BuiltinFunctionInstance allFunc { "all", { FunctionParameter::makeDynamic(&BoolType) }, FunctionParameter::makeFixed(&BoolType) };

namespace code {
    constexpr wrapped_vector::impl<UnresolvedCall<1>> floor(const wrapped_vector& arg) { return { { &floorFunc, { arg.wrappedExpr } } }; }
    constexpr wrapped_scalar::impl<UnresolvedCall<1>> floor(const wrapped_scalar& arg) { return { { &floorFunc, { arg.wrappedExpr } } }; }
    constexpr wrapped_scalar::impl<UnresolvedCall<1>> all(const wrapped_untyped& arg) { return { { &allFunc, { arg.wrappedExpr } } }; }
}

// Composition
struct CompositionExpr : Expr {
    const Expr* components[4] = {};
    int_t componentCount;
    constexpr CompositionExpr(int_t componentCount)
        : Expr(StmtKind::CompositionExpr), componentCount(componentCount) { }
};

namespace code {
    constexpr wrapped_scalar::impl<CompositionExpr> compose(const wrapped_scalar& v) {
        CompositionExpr e = { 1 };
        e.components[0] = v.wrappedExpr;
        return { e };
    }
    constexpr wrapped_vector::impl<CompositionExpr> compose(const wrapped_vector& v) {
        CompositionExpr e = { 1 };
        e.components[0] = v.wrappedExpr;
        return { e };
    }
    constexpr wrapped_vector::impl<CompositionExpr> compose(const wrapped_untyped& v1, const wrapped_untyped& v2) {
        CompositionExpr e = { 2 };
        e.components[0] = v1.wrappedExpr;
        e.components[1] = v2.wrappedExpr;
        return { e };
    }
    constexpr wrapped_vector::impl<CompositionExpr> compose(const wrapped_untyped& v1, const wrapped_untyped& v2, const wrapped_untyped& v3) {
        CompositionExpr e = { 3 };
        e.components[0] = v1.wrappedExpr;
        e.components[1] = v2.wrappedExpr;
        e.components[2] = v3.wrappedExpr;
        return { e };
    }

    constexpr wrapped_vector::impl<CompositionExpr> compose(const wrapped_untyped& v1, const wrapped_untyped& v2, const wrapped_untyped& v3, const wrapped_untyped& v4) {
        CompositionExpr e = { 4 };
        e.components[0] = v1.wrappedExpr;
        e.components[1] = v2.wrappedExpr;
        e.components[2] = v3.wrappedExpr;
        e.components[3] = v4.wrappedExpr;
        return { e };
    }
}

}
