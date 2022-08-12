#pragma once

#include "statements.hpp"
#include "math/Vec4.hpp"

namespace shadergen {

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

constexpr const ScalarType* asScalarType(const Type* type) {
    VERIFY(ShaderGen, type->isScalar);
    return static_cast<const ScalarType*>(type);
}

constexpr const VectorType* asVectorType(const Type* type) {
    VERIFY(ShaderGen, type->isVector);
    return static_cast<const VectorType*>(type);
}

constexpr ScalarType BoolType { "bool", false, true };
constexpr ScalarType IntType { "int", true, false };
constexpr ScalarType FloatType { "float", true, false };

template<>
constexpr const Type* typeFromCXX<bool> = &BoolType;
template<>
constexpr const Type* typeFromCXX<int> = &IntType;
template<>
constexpr const Type* typeFromCXX<float> = &FloatType;
template<>
constexpr const Type* typeFromCXX<Vec2> = FloatType.vectorType(2);
template<>
constexpr const Type* typeFromCXX<Vec3> = FloatType.vectorType(3);
template<>
constexpr const Type* typeFromCXX<Vec4> = FloatType.vectorType(4);

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
