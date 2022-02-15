#pragma once

#include "util/log.hpp"
#include "util/reflection.hpp"
#include "util/types.hpp"
#include "util/uninitialized.hpp"
#include <variant>

namespace shadergen {

// TypeIdentifier
template<typename T>
class IdentifableType;

class TypeIdentifier {
private:
    struct stub { };

    const stub* id;

    constexpr TypeIdentifier(const stub* id)
        : id(id) { }

    template<typename T>
    friend class IdentifableType;

public:
    constexpr bool operator==(const TypeIdentifier& other) const = default;
};

template<typename T>
class IdentifableType {
private:
    static constexpr TypeIdentifier::stub s_identifier;

public:
    static constexpr TypeIdentifier TYPE_ID = { &s_identifier };
};

// StaticVector
template<typename T, int_t cap>
struct StaticVector {
    uninitialized<T> storage[cap];
    int_t m_size = 0;

    constexpr void pushBack(T t) {
        storage[size()].initialize(t);
        m_size++;
    }

    constexpr T& operator[](int_t i) {
        return storage[i].value;
    }
    constexpr const T& operator[](int_t i) const {
        return storage[i].value;
    }

    constexpr int_t size() const { return m_size; }

    constexpr ~StaticVector() {
        for (int_t i = 0; i < size(); i++) {
            storage[i].destroy();
        }
    }
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

template<typename T>
struct TypeTraits {
    static constexpr TypeKind kind = TypeKind::UNKNOWN;
};

template<>
struct TypeTraits<float> {
    static constexpr TypeKind kind = TypeKind::SCALAR;
    static constexpr int_t vectorSize = 1;
    static constexpr ScalarType scalarType = ScalarType::FLOAT;
};

template<>
struct TypeTraits<bool> {
    static constexpr TypeKind kind = TypeKind::SCALAR;
    static constexpr int_t vectorSize = 1;
    static constexpr ScalarType scalarType = ScalarType::BOOL;
};

template<>
struct TypeTraits<int> {
    static constexpr TypeKind kind = TypeKind::SCALAR;
    static constexpr int_t vectorSize = 1;
    static constexpr ScalarType scalarType = ScalarType::INT;
};

template<typename T, int_t size>
struct vec { };

template<typename T, int_t size>
struct TypeTraits<vec<T, size>> {
    static constexpr TypeKind kind = TypeKind::VECTOR;
    static constexpr int_t vectorSize = size;
    static constexpr ScalarType scalarType = TypeTraits<T>::scalarType;
};

template<typename T>
struct Type {
    using Traits = TypeTraits<T>;
    static constexpr Traits traits;
    static constexpr bool isScalar() { return traits.kind == TypeKind::SCALAR; }
    static constexpr bool isVector() { return isScalar() || traits.kind == TypeKind::VECTOR; }
    static constexpr ScalarType scalarType() { return traits.scalarType; }
    static constexpr int_t vectorSize() { return traits.vectorSize; }
    static constexpr bool isArithmetic() {
        return isVector()
            && (scalarType() == ScalarType::FLOAT
                || scalarType() == ScalarType::INT);
    }
    static constexpr bool isLogical() {
        return isVector() && scalarType() == ScalarType::BOOL;
    }
};

// helpers
template<typename Stmt>
void dump(Stmt);

template<ScalarType base>
struct ScalarTypeToCXX_t;
template<>
struct ScalarTypeToCXX_t<ScalarType::BOOL> {
    using type = bool;
};
template<>
struct ScalarTypeToCXX_t<ScalarType::INT> {
    using type = int;
};
template<>
struct ScalarTypeToCXX_t<ScalarType::FLOAT> {
    using type = float;
};
template<ScalarType base>
using ScalarTypeToCXX = typename ScalarTypeToCXX_t<base>::type;

template<ScalarType base, int_t size = 1>
static constexpr auto makeVectorType() {
    if constexpr (size == 1) {
        return Type<ScalarTypeToCXX<base>>();
    } else {
        return Type<vec<ScalarTypeToCXX<base>, size>>();
    }
}
template<typename T, int_t size = 1>
static constexpr auto makeVectorType() {
    return makeVectorType<Type<T>::scalarType(), size>();
}

template<typename T>
struct Expr {
    static constexpr bool IS_EXPR = true;
    static constexpr Type<T> type() { return {}; }
};

template<typename T, fixed_string name_>
struct VarExpr : Expr<T> {
    static constexpr bool IS_VAR_EXPR = true;
    static constexpr std::string_view name() { return name_.view(); }
};

template<typename T, fixed_string name>
static constexpr VarExpr<T, name> makeVariable() { return {}; }
template<fixed_string name, typename T>
static constexpr VarExpr<T, name> makeVariable(Type<T>) { return {}; }

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

template<BinaryOperatorEnum opEnum>
struct BinaryOperator {
    static constexpr BinaryOperatorEnum op() { return opEnum; }
};
template<typename T, BinaryOperatorEnum opEnum, typename Left, typename Right>
struct ElementwiseBinaryOperatorExpr : Expr<T> {
    static constexpr Left left() { return {}; }
    static constexpr Right right() { return {}; }
    static constexpr BinaryOperator<opEnum> op() { return {}; }

    constexpr ElementwiseBinaryOperatorExpr() { }
    constexpr ElementwiseBinaryOperatorExpr(Type<T>, BinaryOperator<opEnum>, Left, Right) { }
};
template<typename T, BinaryOperatorEnum opEnum, typename Vector, typename Scalar, bool scalarIsLeft>
struct VectorByScalarBinaryOperatorExpr : Expr<T> {
    static constexpr Vector vector() { return {}; }
    static constexpr Scalar scalar() { return {}; }
    static constexpr BinaryOperator<opEnum> op() { return {}; }
    static constexpr bool isScalarLeft() { return scalarIsLeft; }

    constexpr VectorByScalarBinaryOperatorExpr() { }
    constexpr VectorByScalarBinaryOperatorExpr(Type<T>, BinaryOperator<opEnum>, Vector, Scalar, std::integral_constant<bool, scalarIsLeft>) { }
};

template<bool b>
static constexpr void constAssert() {
    if constexpr (!b)
        std::abort();
}

template<BinaryOperatorEnum opEnum, typename Left, typename Right>
static constexpr auto buildBinaryOperator(BinaryOperator<opEnum> op, Left left, Right right) {
    if constexpr (isArithmeticOp(op.op())) {
        constAssert<left.type().isArithmetic() && right.type().isArithmetic()>();
        constAssert<left.type().scalarType() == right.type().scalarType()>();
        if constexpr (left.type().vectorSize() == right.type().vectorSize()) {
            return ElementwiseBinaryOperatorExpr(left.type(), op, left, right);
        } else if constexpr (left.type().isScalar()) {
            return VectorByScalarBinaryOperatorExpr(right.type(), op, right, left, std::true_type());
        } else if constexpr (right.type().isScalar()) {
            return VectorByScalarBinaryOperatorExpr(left.type(), op, left, right, std::false_type());
        } else {
            constAssert<false>();
        }
    } else if constexpr (isLogicalOp(op.op())) {
        constAssert<left.type().isLogical() && right.type().isLogical()>();
        constAssert<left.type().scalarType() == right.type().scalarType()>();
        constAssert<left.type().vectorSize() == right.type().vectorSize()>();
        return ElementwiseBinaryOperatorExpr(left.type(), op, left, right, std::false_type());
    } else if constexpr (isComparisonOp(op.op())) {
        constAssert<left.type().isArithmetic() && right.type().isArithmetic()>();
        constAssert<left.type().scalarType() == right.type().scalarType()>();
        constAssert<left.type().vectorSize() == right.type().vectorSize()>();
        return ElementwiseBinaryOperatorExpr(
            makeVectorType<ScalarType::BOOL, left.type().vectorSize()>(),
            op, left, right);
    } else {
        constAssert<false>();
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

template<UnaryOperatorEnum opEnum>
struct UnaryOperator {
    static constexpr UnaryOperatorEnum op() { return opEnum; }
};

template<typename T, UnaryOperatorEnum opEnum, typename SubExpr>
struct UnaryOperatorExpr : Expr<T> {
    static constexpr SubExpr subExpr() { return {}; }
    static constexpr UnaryOperator<opEnum> op() { return {}; }

    constexpr UnaryOperatorExpr() { }
    constexpr UnaryOperatorExpr(Type<T>, UnaryOperator<opEnum>, SubExpr) { }
};

template<UnaryOperatorEnum opEnum, typename SubExpr>
static constexpr auto buildUnaryOperator(UnaryOperator<opEnum> op, SubExpr subExpr) {
    if constexpr (isLogicalOp(op.op())) {
        constAssert<subExpr.type().isLogical()>();
        return UnaryOperatorExpr(subExpr.type(), op, subExpr);
    } else if constexpr (isArithmeticOp(op.op())) {
        constAssert<subExpr.type().isArithmetic()>();
        return UnaryOperatorExpr(subExpr.type(), op, subExpr);
    } else {
        constAssert<false>();
    }
}

namespace code {
    template<typename Left, typename Right>
    constexpr auto operator+(Left left, Right right) { return buildBinaryOperator(BinaryOperator<BinaryOperatorEnum::PLUS>(), left, right); }
    template<typename Left, typename Right>
    constexpr auto operator-(Left left, Right right) { return buildBinaryOperator(BinaryOperator<BinaryOperatorEnum::MINUS>(), left, right); }
    template<typename Left, typename Right>
    constexpr auto operator*(Left left, Right right) { return buildBinaryOperator(BinaryOperator<BinaryOperatorEnum::MULTIPLY>(), left, right); }
    template<typename Left, typename Right>
    constexpr auto operator/(Left left, Right right) { return buildBinaryOperator(BinaryOperator<BinaryOperatorEnum::DIVIDE>(), left, right); }

    template<typename Left, typename Right>
    constexpr auto operator&&(Left left, Right right) { return buildBinaryOperator(BinaryOperator<BinaryOperatorEnum::AND>(), left, right); }
    template<typename Left, typename Right>
    constexpr auto operator||(Left left, Right right) { return buildBinaryOperator(BinaryOperator<BinaryOperatorEnum::OR>(), left, right); }

    template<typename Left, typename Right>
    constexpr auto operator<(Left left, Right right) { return buildBinaryOperator(BinaryOperator<BinaryOperatorEnum::CMP_L>(), left, right); }
    template<typename Left, typename Right>
    constexpr auto operator<=(Left left, Right right) { return buildBinaryOperator(BinaryOperator<BinaryOperatorEnum::CMP_LE>(), left, right); }
    template<typename Left, typename Right>
    constexpr auto operator==(Left left, Right right) { return buildBinaryOperator(BinaryOperator<BinaryOperatorEnum::CMP_EQ>(), left, right); }
    template<typename Left, typename Right>
    constexpr auto operator>=(Left left, Right right) { return buildBinaryOperator(BinaryOperator<BinaryOperatorEnum::CMP_GE>(), left, right); }
    template<typename Left, typename Right>
    constexpr auto operator>(Left left, Right right) { return buildBinaryOperator(BinaryOperator<BinaryOperatorEnum::CMP_G>(), left, right); }
    template<typename Left, typename Right>
    constexpr auto operator!=(Left left, Right right) { return buildBinaryOperator(BinaryOperator<BinaryOperatorEnum::CMP_NE>(), left, right); }

    template<typename SubExpr>
    constexpr auto operator-(SubExpr subExpr) { return buildUnaryOperator(UnaryOperator<UnaryOperatorEnum::NEGATE>(), subExpr); }
    template<typename SubExpr>
    constexpr auto operator!(SubExpr subExpr) { return buildUnaryOperator(UnaryOperator<UnaryOperatorEnum::NOT>(), subExpr); }
}

template<typename Impl>
struct ASTVisitor {
    constexpr Impl* impl() { return static_cast<Impl*>(this); }

    constexpr auto visitExpr(auto) {
        return void();
    }
    constexpr auto visitVarExpr(auto e) {
        return impl()->visitExpr(e);
    }
    constexpr auto visitElementwiseBinaryOperatorExpr(auto e) {
        dispatchVisit(e.left());
        dispatchVisit(e.right());
        return impl()->visitExpr(e);
    }
    constexpr auto visitVectorByScalarBinaryOperatorExpr(auto e) {
        dispatchVisit(e.vector());
        dispatchVisit(e.scalar());
        return impl()->visitExpr(e);
    }

    template<typename E>
    constexpr auto wrap(E e) { return e; }

    // template dispatch
    template<typename T>
    constexpr auto dispatchVisit(Expr<T> e) {
        return impl()->visitExpr(impl()->wrap(e));
    }
    template<typename T, fixed_string name>
    constexpr auto dispatchVisit(VarExpr<T, name> e) {
        return impl()->visitVarExpr(impl()->wrap(e));
    }
    template<typename T, BinaryOperatorEnum op, typename Left, typename Right>
    constexpr auto dispatchVisit(ElementwiseBinaryOperatorExpr<T, op, Left, Right> e) {
        return impl()->visitElementwiseBinaryOperatorExpr(impl()->wrap(e));
    }
    template<typename T, BinaryOperatorEnum op, typename Vector, typename Scalar, bool scalarIsLeft>
    constexpr auto dispatchVisit(VectorByScalarBinaryOperatorExpr<T, op, Vector, Scalar, scalarIsLeft> e) {
        return impl()->visitVectorByScalarBinaryOperatorExpr(impl()->wrap(e));
    }
    template<typename T, UnaryOperatorEnum op, typename SubExpr>
    constexpr auto dispatchVisit(UnaryOperatorExpr<T, op, SubExpr> e) {
        return impl()->visitUnaryOperatorExpr(impl()->wrap(e));
    }
};

// Name and Type validation
static constexpr int_t MAX_NAME_LENGTH = 22;

struct DynamicName {
    char storage[MAX_NAME_LENGTH + 1] = {};
    int8_t m_length = 0;
    template<int_t N>
    constexpr DynamicName(fixed_string<N> str) {
        static_assert(str.length() <= MAX_NAME_LENGTH);
        std::copy(str.data(), str.data() + N, storage);
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

struct TypeInfo {
    size_t alignment = 0;
    int_t sizeInBytes = -1;
    DynamicName name;
    // StaticVector<FieldDecl, 20> fields;
    TypeIdentifier typeId;
};
struct TypeHandle {
    int_t index = -1;
    TypeInfo* ptr;
    constexpr TypeInfo* operator->() {
        return ptr;
    }
    constexpr TypeInfo& operator*() {
        return *ptr;
    }
};
struct TypeContext {
    template<typename T>
    constexpr TypeHandle findType(Type<T>) {
        return {};
    }
};

struct VarDecl {
    DynamicName name;
};
struct VarHandle {
    int_t index = -1;
    VarDecl* ptr = nullptr;
    constexpr VarDecl* operator->() {
        return ptr;
    }
    constexpr VarDecl& operator*() {
        return *ptr;
    }
};

struct VarDeclContext {
    StaticVector<VarDecl, 100> variables;
    template<typename T, fixed_string name>
    constexpr VarHandle findVarDecl(VarExpr<T, name>) {
        for (int_t i = 0; i < variables.size(); i++) {
            if (variables[i].name.view() == name.view()) {
                return { i, &variables[i] };
            }
        }
        variables.pushBack(VarDecl { name });
        return { variables.size() - 1, &variables[variables.size() - 1] };
    }
};

template<typename T>
concept IsExpr = T::IS_EXPR;

template<typename Impl>
struct CheckedASTVisitor : ASTVisitor<Impl> {
    template<typename E>
    struct ExprWrapper : E {
        TypeHandle m_type;
        constexpr TypeHandle typeHandle() { return m_type; };
        constexpr TypeInfo& type() { return *m_type; }
    };
    template<typename E>
    struct VarExprWrapper : ExprWrapper<E> {
        VarHandle m_varDecl;
        constexpr VarHandle varHandle() { return m_varDecl; }
        constexpr VarDecl& varDecl() { return *m_varDecl; }
    };

    constexpr Impl* impl() { return static_cast<Impl*>(this); }

    template<typename E>
    constexpr ExprWrapper<E> wrapExpr(E e) {
        TypeContext& typeContext = impl()->typeContext();
        return ExprWrapper<E> { e, typeContext.findType(e.type()) };
    }

    template<typename E>
    constexpr auto wrap(E e) requires IsExpr<E> {
        return wrapExpr(e);
    }

    template<typename E>
    constexpr auto wrap(E e) requires IsExpr<E> && E::IS_VAR_EXPR {
        VarDeclContext& declContext = impl()->varDeclContext();
        return VarExprWrapper<E> { wrapExpr(e), declContext.findVarDecl(e) };
    }
};

// Persistent AST
struct PersistentAST {
    template<typename T>
    struct NodeHandle {
        constexpr NodeHandle() { }
        template<typename S>
        constexpr NodeHandle(NodeHandle<S> handle) requires std::is_base_of_v<T, S> { }
        constexpr NodeHandle(int_t index)
            : index(index) { }
        int_t index = -1;
    };
    struct ExprNode {
        TypeHandle type;
    };
    struct VarExprNode : ExprNode {
        VarHandle variable;
    };
    struct ElementwiseBinaryOperatorExprNode : ExprNode {
        NodeHandle<ExprNode> left;
        NodeHandle<ExprNode> right;
    };
    using NodeStorage = std::variant<ExprNode, VarExprNode, ElementwiseBinaryOperatorExprNode>;
    StaticVector<NodeStorage, 100> nodes;

    template<typename T>
    constexpr NodeHandle<T> emit(T node) {
        nodes.pushBack(node);
        return { nodes.size() - 1 };
    }

    TypeContext typeContext;
    VarDeclContext varContext;

    struct Builder : CheckedASTVisitor<Builder> {
        PersistentAST& ast;
        constexpr Builder(PersistentAST& ast)
            : ast(ast) { }
        constexpr TypeContext& typeContext() { return ast.typeContext; }
        constexpr VarDeclContext& varDeclContext() { return ast.varContext; }
        constexpr ExprNode visitExpr(auto e) {
            return { e.typeHandle() };
        }
        constexpr VarExprNode visitVarExp(auto e) {
            return { visitExpr(e), e.varHandle() };
        }
        constexpr ElementwiseBinaryOperatorExprNode visitElementwiseBinaryOperatorExpr(auto e) {
            return {
                visitExpr(e),
                ast.emit(dispatchVisit(e.left())),
                ast.emit(dispatchVisit(e.right()))
            };
        }
    };
};

struct ASTDumper : ASTVisitor<ASTDumper> {
    int_t indent = 0;
    template<typename... Args>
    void dump(fmt::format_string<Args...> str, Args&&... args) {
        log("{:{}}{}", "", indent, fmt::format(str, std::forward<Args>(args)...));
    }

    void visitExpr(auto e) {
        dump("{} Expr", e.type());
    }
    void visitVarExpr(auto e) {
        dump("{} VarExpr \"{}\"", e.type(), e.name());
    }
    void visitElementwiseBinaryOperatorExpr(auto e) {
        dump("{} ElementwiseBinaryOperatorExpr '{}'", e.type(), toString(e.op().op()));
        visit(e.left());
        visit(e.right());
    }
    void visitVectorByScalarBinaryOperatorExpr(auto e) {
        dump("{} VectorByScalarBinaryOperatorExpr '{}'", e.type(), toString(e.op().op()));
        visit(e.vector());
        visit(e.scalar());
    }
    void visitUnaryOperatorExpr(auto e) {
        dump("{} UnaryOperatorExpr '{}'", e.type(), toString(e.op().op()));
        visit(e.subExpr());
    }

    void visit(auto e) {
        indent += 2;
        dispatchVisit(e);
        indent -= 2;
    }
};

template<typename Stmt>
void dump(Stmt stmt) {
    ASTDumper dumper;
    dumper.dispatchVisit(stmt);
}
}

template<typename T>
struct fmt::formatter<shadergen::Type<T>> {
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(shadergen::Type<T> type, FormatContext& ctx) {
        if constexpr (type.isScalar()) {
            return format_to(ctx.out(), "[{}]", toString(type.scalarType()));
        } else if constexpr (type.isVector()) {
            return format_to(ctx.out(), "[{} x{}]", toString(type.scalarType()), type.vectorSize());
        } else {
            return format_to(ctx.out(), "[unknown]");
        }
    }
};
