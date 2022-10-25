#pragma once

#include "math/Vec4.hpp"
#include "statements.hpp"

namespace shadergen {

struct ScalarType;

struct MatrixType : NonArrayType {
    const ScalarType* scalarType;
    int_t vectorSize;
    int_t covectorSize;
    bool isArithmetic;
    bool isLogical;

    constexpr const MatrixType* transposedType() const;

    constexpr MatrixType(const ScalarType* scalarType, int_t vectorSize, int_t covectorSize, bool isArithmetic, bool isLogical, bool isScalar = false, bool isVector = false)
        : NonArrayType(false, isScalar, isVector, true, false)
        , scalarType(scalarType)
        , vectorSize(vectorSize)
        , covectorSize(covectorSize)
        , isArithmetic(isArithmetic)
        , isLogical(isLogical) { }
};

struct VectorType : MatrixType {
    constexpr VectorType(const ScalarType* scalarType, int_t vectorSize, bool isArithmetic, bool isLogical, bool isScalar = false)
        : MatrixType(scalarType, vectorSize, 1, isArithmetic, isLogical, isScalar, true) { }
};

struct ScalarType : VectorType {
    VectorType vectorTypes[3] = {
        { this, 2, isArithmetic, isLogical },
        { this, 3, isArithmetic, isLogical },
        { this, 4, isArithmetic, isLogical },
    };
    MatrixType matrixTypes[3][3] = {
        {
            { this, 2, 2, isArithmetic, isLogical },
            { this, 2, 3, isArithmetic, isLogical },
            { this, 2, 4, isArithmetic, isLogical },
        },
        {
            { this, 3, 2, isArithmetic, isLogical },
            { this, 3, 3, isArithmetic, isLogical },
            { this, 3, 4, isArithmetic, isLogical },
        },
        {
            { this, 4, 2, isArithmetic, isLogical },
            { this, 4, 3, isArithmetic, isLogical },
            { this, 4, 4, isArithmetic, isLogical },
        },
    };
    std::string_view name;

    constexpr const VectorType* vectorType(int_t size) const {
        if (size == 1)
            return this;
        return &vectorTypes[size - 2];
    }
    constexpr const MatrixType* matrixType(int_t size, int_t cosize) const {
        if (cosize == 1)
            return vectorType(size);
        if (size == 1)
            return vectorType(cosize);
        return &matrixTypes[size - 2][cosize - 2];
    }

    constexpr ScalarType(std::string_view name, bool isArithmetic, bool isLogical)
        : VectorType(this, 1, isArithmetic, isLogical, true), name(name) { }
};

constexpr const MatrixType* MatrixType::transposedType() const {
    return scalarType->matrixType(covectorSize, vectorSize);
}

constexpr const ArrayType* asArrayType(const Type* type) {
    VERIFY(ShaderGen, type->isArray);
    return static_cast<const ArrayType*>(type);
}
constexpr const ScalarType* asScalarType(const Type* type) {
    VERIFY(ShaderGen, type->isScalar);
    return static_cast<const ScalarType*>(type);
}
constexpr const VectorType* asVectorType(const Type* type) {
    VERIFY(ShaderGen, type->isVector);
    return static_cast<const VectorType*>(type);
}
constexpr const MatrixType* asMatrixType(const Type* type) {
    VERIFY(ShaderGen, type->isMatrix);
    return static_cast<const MatrixType*>(type);
}

constexpr Type VoidType { false, false, false, false, false, false };
constexpr ScalarType BoolType { "bool", false, true };
constexpr ScalarType IntType { "int", true, false };
constexpr ScalarType UIntType { "uint", true, false };
constexpr ScalarType FloatType { "float", true, false };

struct iVec2 { };
struct iVec3 { };
struct iVec4 { };
struct uVec2 { };
struct uVec3 { };
struct uVec4 { };

template<>
constexpr const Type* typeFromCXX<bool> = &BoolType;
template<>
constexpr const Type* typeFromCXX<int> = &IntType;
template<>
constexpr const Type* typeFromCXX<unsigned> = &UIntType;
template<>
constexpr const Type* typeFromCXX<float> = &FloatType;
template<>
constexpr const Type* typeFromCXX<Vec2> = FloatType.vectorType(2);
template<>
constexpr const Type* typeFromCXX<Vec3> = FloatType.vectorType(3);
template<>
constexpr const Type* typeFromCXX<Vec4> = FloatType.vectorType(4);
template<>
constexpr const Type* typeFromCXX<Mat4x4> = FloatType.matrixType(4, 4);
template<>
constexpr const Type* typeFromCXX<iVec2> = IntType.vectorType(2);
template<>
constexpr const Type* typeFromCXX<iVec3> = IntType.vectorType(3);
template<>
constexpr const Type* typeFromCXX<iVec4> = IntType.vectorType(4);
template<>
constexpr const Type* typeFromCXX<uVec2> = UIntType.vectorType(2);
template<>
constexpr const Type* typeFromCXX<uVec3> = UIntType.vectorType(3);
template<>
constexpr const Type* typeFromCXX<uVec4> = UIntType.vectorType(4);

struct wrapped_untyped {
    const Expr* wrappedExpr;
};

// Scalars
struct wrapped_scalar : wrapped_untyped {
    template<typename E>
    struct impl;
};
template<typename E>
struct wrapped_scalar::impl : E, wrapped_scalar {
    using wrapper_base = wrapped_scalar;
    constexpr impl(const E& e)
        : E(e), wrapped_scalar { (Expr*)this } { }

    constexpr AssignmentStmt operator=(const wrapped_scalar::impl<E>& right) const {
        return { this, right.wrappedExpr };
    }
    constexpr AssignmentStmt operator=(const wrapped_scalar& right) const {
        return { this, right.wrappedExpr };
    }
};

// Arrays
struct ArraySubscriptExpr : Expr {
    const Expr* base;
    const Expr* index;
    constexpr ArraySubscriptExpr(const Expr* base, const Expr* index)
        : Expr(StmtKind::ArraySubscriptExpr), base(base), index(index) { }
};

template<typename next_wrapper>
struct wrapped_array : wrapped_untyped {
    template<typename E>
    struct impl;
};
template<typename next_wrapper>
template<typename E>
struct wrapped_array<next_wrapper>::impl : E, wrapped_array {
    using wrapper_base = wrapped_array;
    constexpr impl(const E& e)
        : E(e), wrapped_array { (Expr*)this } { }

    constexpr typename next_wrapper::template impl<ArraySubscriptExpr> operator[](const wrapped_scalar& index) const {
        return { { this, index.wrappedExpr } };
    }
};

// Matrices
struct wrapped_matrix : wrapped_untyped {
    template<typename E>
    struct impl;
};
template<typename E>
struct wrapped_matrix::impl : E, wrapped_matrix {
    using wrapper_base = wrapped_matrix;
    constexpr impl(const E& e)
        : E(e), wrapped_matrix { (Expr*)this } { }

    constexpr AssignmentStmt operator=(const wrapped_matrix::impl<E>& right) const {
        return { this, right.wrappedExpr };
    }
    constexpr AssignmentStmt operator=(const wrapped_matrix& right) const {
        return { this, right.wrappedExpr };
    }
};

// Vectors
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

    constexpr wrapped_vector::impl<VectorComponentExpr> xx() const;
    constexpr wrapped_vector::impl<VectorComponentExpr> xy() const;
    constexpr wrapped_vector::impl<VectorComponentExpr> xz() const;
    constexpr wrapped_vector::impl<VectorComponentExpr> xw() const;

    constexpr wrapped_vector::impl<VectorComponentExpr> yx() const;
    constexpr wrapped_vector::impl<VectorComponentExpr> yy() const;
    constexpr wrapped_vector::impl<VectorComponentExpr> yz() const;
    constexpr wrapped_vector::impl<VectorComponentExpr> yw() const;

    constexpr wrapped_vector::impl<VectorComponentExpr> zx() const;
    constexpr wrapped_vector::impl<VectorComponentExpr> zy() const;
    constexpr wrapped_vector::impl<VectorComponentExpr> zz() const;
    constexpr wrapped_vector::impl<VectorComponentExpr> zw() const;

    constexpr wrapped_vector::impl<VectorComponentExpr> wx() const;
    constexpr wrapped_vector::impl<VectorComponentExpr> wy() const;
    constexpr wrapped_vector::impl<VectorComponentExpr> wz() const;
    constexpr wrapped_vector::impl<VectorComponentExpr> ww() const;

    constexpr wrapped_vector::impl<VectorComponentExpr> xxx() const;
    constexpr wrapped_vector::impl<VectorComponentExpr> xxy() const;
    constexpr wrapped_vector::impl<VectorComponentExpr> xxz() const;
    constexpr wrapped_vector::impl<VectorComponentExpr> xxw() const;

    constexpr wrapped_vector::impl<VectorComponentExpr> xyx() const;
    constexpr wrapped_vector::impl<VectorComponentExpr> xyy() const;
    constexpr wrapped_vector::impl<VectorComponentExpr> xyz() const;
    constexpr wrapped_vector::impl<VectorComponentExpr> xyw() const;

    constexpr wrapped_vector::impl<VectorComponentExpr> xzx() const;
    constexpr wrapped_vector::impl<VectorComponentExpr> xzy() const;
    constexpr wrapped_vector::impl<VectorComponentExpr> xzz() const;
    constexpr wrapped_vector::impl<VectorComponentExpr> xzw() const;

    constexpr wrapped_vector::impl<VectorComponentExpr> xwx() const;
    constexpr wrapped_vector::impl<VectorComponentExpr> xwy() const;
    constexpr wrapped_vector::impl<VectorComponentExpr> xwz() const;
    constexpr wrapped_vector::impl<VectorComponentExpr> xww() const;

    constexpr wrapped_vector::impl<VectorComponentExpr> yxx() const;
    constexpr wrapped_vector::impl<VectorComponentExpr> yxy() const;
    constexpr wrapped_vector::impl<VectorComponentExpr> yxz() const;
    constexpr wrapped_vector::impl<VectorComponentExpr> yxw() const;

    constexpr wrapped_vector::impl<VectorComponentExpr> yyx() const;
    constexpr wrapped_vector::impl<VectorComponentExpr> yyy() const;
    constexpr wrapped_vector::impl<VectorComponentExpr> yyz() const;
    constexpr wrapped_vector::impl<VectorComponentExpr> yyw() const;

    constexpr wrapped_vector::impl<VectorComponentExpr> yzx() const;
    constexpr wrapped_vector::impl<VectorComponentExpr> yzy() const;
    constexpr wrapped_vector::impl<VectorComponentExpr> yzz() const;
    constexpr wrapped_vector::impl<VectorComponentExpr> yzw() const;

    constexpr wrapped_vector::impl<VectorComponentExpr> ywx() const;
    constexpr wrapped_vector::impl<VectorComponentExpr> ywy() const;
    constexpr wrapped_vector::impl<VectorComponentExpr> ywz() const;
    constexpr wrapped_vector::impl<VectorComponentExpr> yww() const;

    constexpr wrapped_vector::impl<VectorComponentExpr> zxx() const;
    constexpr wrapped_vector::impl<VectorComponentExpr> zxy() const;
    constexpr wrapped_vector::impl<VectorComponentExpr> zxz() const;
    constexpr wrapped_vector::impl<VectorComponentExpr> zxw() const;

    constexpr wrapped_vector::impl<VectorComponentExpr> zyx() const;
    constexpr wrapped_vector::impl<VectorComponentExpr> zyy() const;
    constexpr wrapped_vector::impl<VectorComponentExpr> zyz() const;
    constexpr wrapped_vector::impl<VectorComponentExpr> zyw() const;

    constexpr wrapped_vector::impl<VectorComponentExpr> zzx() const;
    constexpr wrapped_vector::impl<VectorComponentExpr> zzy() const;
    constexpr wrapped_vector::impl<VectorComponentExpr> zzz() const;
    constexpr wrapped_vector::impl<VectorComponentExpr> zzw() const;

    constexpr wrapped_vector::impl<VectorComponentExpr> zwx() const;
    constexpr wrapped_vector::impl<VectorComponentExpr> zwy() const;
    constexpr wrapped_vector::impl<VectorComponentExpr> zwz() const;
    constexpr wrapped_vector::impl<VectorComponentExpr> zww() const;

    constexpr wrapped_vector::impl<VectorComponentExpr> wxx() const;
    constexpr wrapped_vector::impl<VectorComponentExpr> wxy() const;
    constexpr wrapped_vector::impl<VectorComponentExpr> wxz() const;
    constexpr wrapped_vector::impl<VectorComponentExpr> wxw() const;

    constexpr wrapped_vector::impl<VectorComponentExpr> wyx() const;
    constexpr wrapped_vector::impl<VectorComponentExpr> wyy() const;
    constexpr wrapped_vector::impl<VectorComponentExpr> wyz() const;
    constexpr wrapped_vector::impl<VectorComponentExpr> wyw() const;

    constexpr wrapped_vector::impl<VectorComponentExpr> wzx() const;
    constexpr wrapped_vector::impl<VectorComponentExpr> wzy() const;
    constexpr wrapped_vector::impl<VectorComponentExpr> wzz() const;
    constexpr wrapped_vector::impl<VectorComponentExpr> wzw() const;

    constexpr wrapped_vector::impl<VectorComponentExpr> wwx() const;
    constexpr wrapped_vector::impl<VectorComponentExpr> wwy() const;
    constexpr wrapped_vector::impl<VectorComponentExpr> wwz() const;
    constexpr wrapped_vector::impl<VectorComponentExpr> www() const;
};
template<typename E>
struct wrapped_vector::impl : E, wrapped_vector {
    using wrapper_base = wrapped_vector;
    constexpr impl(const E& e)
        : E(e), wrapped_vector { (E*)this } { }

    constexpr AssignmentStmt operator=(const impl<E>& right) const {
        return { this, right.wrappedExpr };
    }
    constexpr AssignmentStmt operator=(const wrapped_vector& right) const {
        return { this, right.wrappedExpr };
    }
};

constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::xx() const { return { { wrappedExpr, { 0, 0 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::xy() const { return { { wrappedExpr, { 0, 1 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::xz() const { return { { wrappedExpr, { 0, 2 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::xw() const { return { { wrappedExpr, { 0, 3 } } }; }

constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::yx() const { return { { wrappedExpr, { 1, 0 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::yy() const { return { { wrappedExpr, { 1, 1 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::yz() const { return { { wrappedExpr, { 1, 2 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::yw() const { return { { wrappedExpr, { 1, 3 } } }; }

constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::zx() const { return { { wrappedExpr, { 2, 0 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::zy() const { return { { wrappedExpr, { 2, 1 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::zz() const { return { { wrappedExpr, { 2, 2 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::zw() const { return { { wrappedExpr, { 2, 3 } } }; }

constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::wx() const { return { { wrappedExpr, { 3, 0 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::wy() const { return { { wrappedExpr, { 3, 1 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::wz() const { return { { wrappedExpr, { 3, 2 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::ww() const { return { { wrappedExpr, { 3, 3 } } }; }

constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::xxx() const { return { { wrappedExpr, { 0, 0, 0 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::xxy() const { return { { wrappedExpr, { 0, 0, 1 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::xxz() const { return { { wrappedExpr, { 0, 0, 2 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::xxw() const { return { { wrappedExpr, { 0, 0, 3 } } }; }

constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::xyx() const { return { { wrappedExpr, { 0, 1, 0 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::xyy() const { return { { wrappedExpr, { 0, 1, 1 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::xyz() const { return { { wrappedExpr, { 0, 1, 2 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::xyw() const { return { { wrappedExpr, { 0, 1, 3 } } }; }

constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::xzx() const { return { { wrappedExpr, { 0, 2, 0 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::xzy() const { return { { wrappedExpr, { 0, 2, 1 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::xzz() const { return { { wrappedExpr, { 0, 2, 2 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::xzw() const { return { { wrappedExpr, { 0, 2, 3 } } }; }

constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::xwx() const { return { { wrappedExpr, { 0, 3, 0 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::xwy() const { return { { wrappedExpr, { 0, 3, 1 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::xwz() const { return { { wrappedExpr, { 0, 3, 2 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::xww() const { return { { wrappedExpr, { 0, 3, 3 } } }; }

constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::yxx() const { return { { wrappedExpr, { 1, 0, 0 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::yxy() const { return { { wrappedExpr, { 1, 0, 1 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::yxz() const { return { { wrappedExpr, { 1, 0, 2 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::yxw() const { return { { wrappedExpr, { 1, 0, 3 } } }; }

constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::yyx() const { return { { wrappedExpr, { 1, 1, 0 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::yyy() const { return { { wrappedExpr, { 1, 1, 1 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::yyz() const { return { { wrappedExpr, { 1, 1, 2 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::yyw() const { return { { wrappedExpr, { 1, 1, 3 } } }; }

constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::yzx() const { return { { wrappedExpr, { 1, 2, 0 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::yzy() const { return { { wrappedExpr, { 1, 2, 1 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::yzz() const { return { { wrappedExpr, { 1, 2, 2 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::yzw() const { return { { wrappedExpr, { 1, 2, 3 } } }; }

constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::ywx() const { return { { wrappedExpr, { 1, 3, 0 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::ywy() const { return { { wrappedExpr, { 1, 3, 1 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::ywz() const { return { { wrappedExpr, { 1, 3, 2 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::yww() const { return { { wrappedExpr, { 1, 3, 3 } } }; }

constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::zxx() const { return { { wrappedExpr, { 2, 0, 0 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::zxy() const { return { { wrappedExpr, { 2, 0, 1 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::zxz() const { return { { wrappedExpr, { 2, 0, 2 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::zxw() const { return { { wrappedExpr, { 2, 0, 3 } } }; }

constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::zyx() const { return { { wrappedExpr, { 2, 1, 0 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::zyy() const { return { { wrappedExpr, { 2, 1, 1 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::zyz() const { return { { wrappedExpr, { 2, 1, 2 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::zyw() const { return { { wrappedExpr, { 2, 1, 3 } } }; }

constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::zzx() const { return { { wrappedExpr, { 2, 2, 0 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::zzy() const { return { { wrappedExpr, { 2, 2, 1 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::zzz() const { return { { wrappedExpr, { 2, 2, 2 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::zzw() const { return { { wrappedExpr, { 2, 2, 3 } } }; }

constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::zwx() const { return { { wrappedExpr, { 2, 3, 0 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::zwy() const { return { { wrappedExpr, { 2, 3, 1 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::zwz() const { return { { wrappedExpr, { 2, 3, 2 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::zww() const { return { { wrappedExpr, { 2, 3, 3 } } }; }

constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::wxx() const { return { { wrappedExpr, { 3, 0, 0 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::wxy() const { return { { wrappedExpr, { 3, 0, 1 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::wxz() const { return { { wrappedExpr, { 3, 0, 2 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::wxw() const { return { { wrappedExpr, { 3, 0, 3 } } }; }

constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::wyx() const { return { { wrappedExpr, { 3, 1, 0 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::wyy() const { return { { wrappedExpr, { 3, 1, 1 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::wyz() const { return { { wrappedExpr, { 3, 1, 2 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::wyw() const { return { { wrappedExpr, { 3, 1, 3 } } }; }

constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::wzx() const { return { { wrappedExpr, { 3, 2, 0 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::wzy() const { return { { wrappedExpr, { 3, 2, 1 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::wzz() const { return { { wrappedExpr, { 3, 2, 2 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::wzw() const { return { { wrappedExpr, { 3, 2, 3 } } }; }

constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::wwx() const { return { { wrappedExpr, { 3, 3, 0 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::wwy() const { return { { wrappedExpr, { 3, 3, 1 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::wwz() const { return { { wrappedExpr, { 3, 3, 2 } } }; }
constexpr wrapped_vector::impl<VectorComponentExpr> wrapped_vector::www() const { return { { wrappedExpr, { 3, 3, 3 } } }; }

// Literals
union LiteralStorage {
    bool bools[4];
    int ints[4];
    float floats[4];
    unsigned uints[4];

    constexpr LiteralStorage() { }
    template<int_t N>
    constexpr LiteralStorage(const bool (&in)[N])
        : bools() { std::copy_n(in, N, bools); }
    template<int_t N>
    constexpr LiteralStorage(const int (&in)[N])
        : ints() { std::copy_n(in, N, ints); }
    template<int_t N>
    constexpr LiteralStorage(const unsigned (&in)[N])
        : uints() { std::copy_n(in, N, uints); }
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
