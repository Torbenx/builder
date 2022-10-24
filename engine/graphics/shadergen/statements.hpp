#pragma once

#include "containers/ArrayView.hpp"
#include "util/log.hpp"
#include "util/types.hpp"
#include <array>
#include <concepts>
#include <string_view>
#include <type_traits>

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
    bool isMatrix;
    bool isArray;
    bool isError;
    constexpr Type(bool isUser, bool isScalar, bool isVector, bool isMatrix, bool isArray, bool isError)
        : isUser(isUser), isScalar(isScalar), isVector(isVector), isMatrix(isMatrix), isArray(isArray), isError(isError) { }
};
struct NonArrayType;
struct ArrayType : Type {
    const NonArrayType* baseType;
    constexpr ArrayType(const NonArrayType* baseType)
        : Type(false, false, false, false, true, false)
        , baseType(baseType) { }
};
struct NonArrayType : Type {
    ArrayType m_arrayType;
    constexpr const ArrayType* arrayType() const { return &m_arrayType; }
    constexpr NonArrayType(bool isUser, bool isScalar, bool isVector, bool isMatrix, bool isError)
        : Type(isUser, isScalar, isVector, isMatrix, false, isError)
        , m_arrayType(this) { }
    constexpr NonArrayType(const NonArrayType& other)
        : Type(other), m_arrayType(this) { }
};

template<typename T>
constexpr const Type* typeFromCXX = &T::SHADERGEN_TYPE;
template<typename T>
constexpr const Type* typeFromCXX<T[]> = static_cast<const NonArrayType*>(typeFromCXX<T>)->arrayType();

// Statements
// clang-format off
#define ENUMERATE_STMT_KINDS(callback)   \
    callback(Stmt)                       \
    callback(Expr)                       \
    callback(VarRefExpr)                 \
    callback(UnaryOperatorExpr)          \
    callback(MemberAccessExpr)           \
    callback(ArraySubscriptExpr)         \
    callback(ElementwiseOperatorExpr)    \
    callback(VectorByScalarOperatorExpr) \
    callback(MatrixMultiplicationExpr)   \
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
    callback(UpdateStmt)                 \
    callback(ParenExpr)

#define ENUMERATE_AST_NODES(callback) \
    ENUMERATE_STMT_KINDS(callback)    \
    callback(ASTListEntry)            \
    callback(VarDecl)                 \
    callback(ErrorNote)               \
    callback(StageDecl)
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
constexpr T* castStmt(Stmt* stmt) {
    return (T*)stmt;
}
template<typename T>
constexpr const T* castStmt(const Stmt* stmt) {
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
    int32_t entryCount = 0;
    bool isLinked : 1 = true;

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
    constexpr bool empty() const {
        return entryCount == 0;
    }
    constexpr T* front() const {
        if (isLinked)
            return (T*)linked.first->node;
        else
            return array.begin[0];
    }
    constexpr T* back() const {
        if (isLinked)
            return (T*)linked.last->node;
        else
            return array.begin[entryCount - 1];
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
    template<typename Callback>
    constexpr void selectEntries(Callback&& callback) {
        if (isLinked) {
            ASTListEntry** prevNextPtr = &linked.first;
            while (ASTListEntry* current = *prevNextPtr) {
                if (callback((T*)current->node)) {
                    // keep it
                    prevNextPtr = &current->next;
                } else {
                    // lose it
                    *prevNextPtr = current->next;
                    current->next = nullptr;
                    current->node = nullptr;
                    entryCount -= 1;
                }
            }
        } else {
            for (int_t i = 0; i < entryCount; i++) {
                if (callback(array.begin[i]))
                    continue; // keep it
                // lose it
                std::copy(&array.begin[i + 1], &array.begin[entryCount], &array.begin[i]);
                i -= 1;
                entryCount -= 1;
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
    ValueKind valueKind = ValueKind::UNKNOWN;
    const Type* type = nullptr;
    constexpr Expr(StmtKind kind)
        : Stmt(kind) { }
};

struct ParenExpr : Expr {
    const Expr* subExpr;
    constexpr ParenExpr(const Expr* subExpr)
        : Expr(StmtKind::ParenExpr), subExpr(subExpr) { }
};

// Errors
constexpr Type ErrorType = Type(false, false, false, false, false, true);
struct ErrorNote : ASTNode {
    std::string_view message;
};
struct ErrorNode {
    std::string_view message;
    ASTList<ErrorNote> notes;

    constexpr bool hasError() { return !message.empty(); }
    constexpr void clear() {
        message = {};
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
struct VarDeclModifier : ASTNode {
    std::string_view name;
    bool temporary = false;
    constexpr VarDeclModifier() = default;
    constexpr VarDeclModifier(std::string_view name, bool temporary = false)
        : name(name), temporary(temporary) { }
};
struct VarDecl : ASTNode {
    std::string_view name = {};
    const Type* type = nullptr;
    const VarDeclContext* declContext = nullptr;
    const VarDecl* prevDecl = nullptr;
    ASTList<const VarDeclModifier> modifiers;
    // FixedBitset initMask = 0;
    // FixedBitset usedMask = 0;
    constexpr VarDecl() = default;
    constexpr VarDecl(std::string_view name, const Type* type, const VarDeclContext* declContext)
        : name(name), type(type), declContext(declContext) { }
};
struct VarDeclContext {
    VarDeclContext* parent = nullptr;
    ASTList<VarDecl> decls;
};
constexpr VarDeclContext globalPseudoVarDeclContext {};
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

// Control
struct CompoundStmt : Stmt, ASTList<Stmt> {
    constexpr CompoundStmt()
        : Stmt(StmtKind::CompoundStmt) { }
};

struct BlockStmt : Stmt {
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

// Shader Stages
struct StageDecl : ASTNode {
    VarDeclContext globals;
    Stmt* entryPoint = nullptr;
};

}
