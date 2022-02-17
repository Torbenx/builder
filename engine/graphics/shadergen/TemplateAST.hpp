#pragma once

#include "AST.hpp"
#include "util/reflection.hpp"

namespace shadergen {

// Types
template<typename T>
struct TypeTraits {
    static constexpr TypeKind kind = TypeKind::USER;
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
struct vec {
    T elements[size];
};

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
    static constexpr bool isBuiltin() { return traits.kind != TypeKind::USER; }
    static constexpr bool isScalar() { return traits.kind == TypeKind::SCALAR; }
    static constexpr bool isVector() { return isScalar() || traits.kind == TypeKind::VECTOR; }
    static constexpr ScalarType scalarType() {
        if constexpr (isBuiltin())
            return traits.scalarType;
        return ScalarType::UNKNOWN;
    }
    static constexpr int_t vectorSize() {
        if constexpr (isBuiltin())
            return traits.vectorSize;
        return 0;
    }
    static constexpr bool isArithmetic() {
        return isVector()
            && (scalarType() == ScalarType::FLOAT
                || scalarType() == ScalarType::INT);
    }
    static constexpr bool isLogical() {
        return isVector() && scalarType() == ScalarType::BOOL;
    }
};

template<typename T1, typename T2>
static constexpr bool isSameType(Type<T1>, Type<T2>) {
    return std::is_same_v<T1, T2>;
}

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

// Expr
template<typename T, ValueKind valueKind_ = ValueKind::RVALUE>
struct Expr {
    using CXXType = T;
    static constexpr Type<T> type() { return {}; }
    static constexpr ValueKind valueKind() { return valueKind_; }
    static constexpr bool isLValue() { return valueKind() == ValueKind::LVALUE; }
};

// Variables
template<typename T, fixed_string name_>
struct VarExpr : Expr<T, ValueKind::LVALUE> {
    VarHandle varHandle;
    constexpr VarExpr(VarHandle varHandle)
        : varHandle(varHandle) { }
    static constexpr std::string_view name() { return name_.view(); }
};

// Field access
template<typename Base, typename T, FieldInfo fieldInfo>
struct FieldAccessExpr : Expr<T, Base::valueKind()> {
    Base base;
    static constexpr int_t fieldIndex() { return fieldInfo.index; }
    constexpr FieldAccessExpr(Base base)
        : base(base) { }
    static constexpr std::string_view fieldName() { return fieldInfo.name.view(); }
};

template<typename Left, typename Right>
struct AssignmentStmt {
    Left left;
    Right right;
    constexpr AssignmentStmt(Left left, Right right)
        : left(left), right(right) { }
};

template<typename Left, typename Right>
static constexpr auto buildAssignment(Left left, Right right) {
    VERIFY(ShaderGen, isSameType(left.type(), right.type()));
    VERIFY(ShaderGen, left.isLValue());
    return AssignmentStmt(left, right);
}

template<typename E>
struct wrapped;

template<typename E>
constexpr E unwrap(wrapped<E> wrapped) { return (E)wrapped; }
template<typename E>
constexpr wrapped<E> wrap(E e) { return { e }; }

template<typename Base, typename T, FieldInfo fieldInfo>
using WrappedFieldAccessExpr = wrapped<FieldAccessExpr<Base, T, fieldInfo>>;
template<typename E>
struct wrapped : E {
    constexpr wrapped(const E& e)
        : E(e) { }
};
template<typename E>
requires requires { typename E::CXXType; }
struct wrapped<E> : E, ref_fields_or_empty<typename E::CXXType, WrappedFieldAccessExpr, E> {
    constexpr wrapped(const E& e)
        : E(e), ref_fields_or_empty<typename E::CXXType, WrappedFieldAccessExpr, E>(e) { }
    constexpr auto operator=(wrapped<E> right) { return wrap(buildAssignment(unwrap(*this), unwrap(right))); }
    template<typename Right>
    constexpr auto operator=(Right right) { return wrap(buildAssignment(unwrap(*this), unwrap(right))); }
};

template<typename T, fixed_string name>
static constexpr VarExpr<T, name> makeVariable() { return {}; }
template<fixed_string name, typename T>
static constexpr VarExpr<T, name> makeVariable(Type<T>) { return {}; }

// Binary operators
template<BinaryOperatorEnum opEnum>
struct BinaryOperator {
    static constexpr BinaryOperatorEnum op() { return opEnum; }
};
template<typename T, BinaryOperatorEnum opEnum, typename Left, typename Right>
struct ElementwiseBinaryOperatorExpr : Expr<T> {
    Left left;
    Right right;
    static constexpr BinaryOperatorEnum op() { return opEnum; }

    constexpr ElementwiseBinaryOperatorExpr(Type<T>, BinaryOperator<opEnum>, Left left, Right right)
        : left(left), right(right) { }
};
template<typename T, BinaryOperatorEnum opEnum, typename Vector, typename Scalar, bool scalarIsLeft>
struct VectorByScalarBinaryOperatorExpr : Expr<T> {
    Vector vector;
    Scalar scalar;
    static constexpr BinaryOperatorEnum op() { return opEnum; }
    static constexpr bool isScalarLeft() { return scalarIsLeft; }

    constexpr VectorByScalarBinaryOperatorExpr(Type<T>, BinaryOperator<opEnum>, Vector vector, Scalar scalar, std::bool_constant<scalarIsLeft>)
        : vector(vector), scalar(scalar) { }
};

template<BinaryOperatorEnum opEnum, typename Left, typename Right>
static constexpr auto buildBinaryOperator(BinaryOperator<opEnum> op, Left left, Right right) {
    if constexpr (isArithmeticOp(op.op())) {
        VERIFY(ShaderGen, left.type().isArithmetic() && right.type().isArithmetic());
        VERIFY(ShaderGen, left.type().scalarType() == right.type().scalarType());
        if constexpr (left.type().vectorSize() == right.type().vectorSize()) {
            return ElementwiseBinaryOperatorExpr(left.type(), op, left, right);
        } else if constexpr (left.type().isScalar()) {
            return VectorByScalarBinaryOperatorExpr(right.type(), op, right, left, std::true_type());
        } else if constexpr (right.type().isScalar()) {
            return VectorByScalarBinaryOperatorExpr(left.type(), op, left, right, std::false_type());
        } else {
            VERIFY_NOT_REACHED(ShaderGen);
        }
    } else if constexpr (isLogicalOp(op.op())) {
        VERIFY(ShaderGen, left.type().isLogical() && right.type().isLogical());
        VERIFY(ShaderGen, left.type().scalarType() == right.type().scalarType());
        VERIFY(ShaderGen, left.type().vectorSize() == right.type().vectorSize());
        return ElementwiseBinaryOperatorExpr(left.type(), op, left, right);
    } else if constexpr (isComparisonOp(op.op())) {
        VERIFY(ShaderGen, left.type().isArithmetic() && right.type().isArithmetic());
        VERIFY(ShaderGen, left.type().scalarType() == right.type().scalarType());
        VERIFY(ShaderGen, left.type().vectorSize() == right.type().vectorSize());
        return ElementwiseBinaryOperatorExpr(
            makeVectorType<ScalarType::BOOL, left.type().vectorSize()>(), op, left, right);
    } else {
        VERIFY_NOT_REACHED(ShaderGen);
    }
}

namespace code {
    template<typename Left, typename Right>
    constexpr auto operator+(Left left, Right right) { return wrap(buildBinaryOperator(BinaryOperator<BinaryOperatorEnum::PLUS>(), unwrap(left), unwrap(right))); }
    template<typename Left, typename Right>
    constexpr auto operator-(Left left, Right right) { return wrap(buildBinaryOperator(BinaryOperator<BinaryOperatorEnum::MINUS>(), unwrap(left), unwrap(right))); }
    template<typename Left, typename Right>
    constexpr auto operator*(Left left, Right right) { return wrap(buildBinaryOperator(BinaryOperator<BinaryOperatorEnum::MULTIPLY>(), unwrap(left), unwrap(right))); }
    template<typename Left, typename Right>
    constexpr auto operator/(Left left, Right right) { return wrap(buildBinaryOperator(BinaryOperator<BinaryOperatorEnum::DIVIDE>(), unwrap(left), unwrap(right))); }

    template<typename Left, typename Right>
    constexpr auto operator&&(Left left, Right right) { return wrap(buildBinaryOperator(BinaryOperator<BinaryOperatorEnum::AND>(), unwrap(left), unwrap(right))); }
    template<typename Left, typename Right>
    constexpr auto operator||(Left left, Right right) { return wrap(buildBinaryOperator(BinaryOperator<BinaryOperatorEnum::OR>(), unwrap(left), unwrap(right))); }

    template<typename Left, typename Right>
    constexpr auto operator<(Left left, Right right) { return wrap(buildBinaryOperator(BinaryOperator<BinaryOperatorEnum::CMP_L>(), unwrap(left), unwrap(right))); }
    template<typename Left, typename Right>
    constexpr auto operator<=(Left left, Right right) { return wrap(buildBinaryOperator(BinaryOperator<BinaryOperatorEnum::CMP_LE>(), unwrap(left), unwrap(right))); }
    template<typename Left, typename Right>
    constexpr auto operator==(Left left, Right right) { return wrap(buildBinaryOperator(BinaryOperator<BinaryOperatorEnum::CMP_EQ>(), unwrap(left), unwrap(right))); }
    template<typename Left, typename Right>
    constexpr auto operator>=(Left left, Right right) { return wrap(buildBinaryOperator(BinaryOperator<BinaryOperatorEnum::CMP_GE>(), unwrap(left), unwrap(right))); }
    template<typename Left, typename Right>
    constexpr auto operator>(Left left, Right right) { return wrap(buildBinaryOperator(BinaryOperator<BinaryOperatorEnum::CMP_G>(), unwrap(left), unwrap(right))); }
    template<typename Left, typename Right>
    constexpr auto operator!=(Left left, Right right) { return wrap(buildBinaryOperator(BinaryOperator<BinaryOperatorEnum::CMP_NE>(), unwrap(left), unwrap(right))); }
}

// Unary operators
template<UnaryOperatorEnum opEnum>
struct UnaryOperator {
    static constexpr UnaryOperatorEnum op() { return opEnum; }
};

template<typename T, UnaryOperatorEnum opEnum, typename SubExpr>
struct UnaryOperatorExpr : Expr<T> {
    SubExpr subExpr;
    static constexpr UnaryOperatorEnum op() { return opEnum; }

    constexpr UnaryOperatorExpr(Type<T>, UnaryOperator<opEnum>, SubExpr subExpr)
        : subExpr(subExpr) { }
};

template<UnaryOperatorEnum opEnum, typename SubExpr>
static constexpr auto buildUnaryOperator(UnaryOperator<opEnum> op, SubExpr subExpr) {
    if constexpr (isLogicalOp(op.op())) {
        VERIFY(ShaderGen, subExpr.type().isLogical());
        return UnaryOperatorExpr(subExpr.type(), op, subExpr);
    } else if constexpr (isArithmeticOp(op.op())) {
        VERIFY(ShaderGen, subExpr.type().isArithmetic());
        return UnaryOperatorExpr(subExpr.type(), op, subExpr);
    } else {
        VERIFY_NOT_REACHED(ShaderGen);
    }
}

namespace code {
    template<typename SubExpr>
    constexpr auto operator-(SubExpr subExpr) { return wrap(buildUnaryOperator(UnaryOperator<UnaryOperatorEnum::NEGATE>(), unwrap(subExpr))); }
    template<typename SubExpr>
    constexpr auto operator!(SubExpr subExpr) { return wrap(buildUnaryOperator(UnaryOperator<UnaryOperatorEnum::NOT>(), unwrap(subExpr))); }
}

// Literals
template<typename T>
struct LiteralExpr : Expr<T> {
    T value;
    constexpr LiteralExpr(T value)
        : value(value) { }
};

}
