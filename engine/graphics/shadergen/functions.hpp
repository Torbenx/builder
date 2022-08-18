#pragma once

#include "user_types.hpp"

namespace shadergen {

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
    } u;
    static constexpr FunctionParameter fixed(const Type* type) {
        return { .isDynamic = false, .u = { .fixed = { type } } };
    }
    static constexpr FunctionParameter dynamic(const ScalarType* baseType) {
        return { .isDynamic = true, .u = { .dynamic = { baseType } } };
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
    constexpr FunctionGroup(std::string_view name, ArrayView<const FunctionInstance*> instances)
        : Function(true, name), instances(instances) { }
};
template<int_t N>
struct BuiltinFunctionGroup : FunctionGroup {
    const FunctionInstance* insts[N];
    constexpr BuiltinFunctionGroup(std::string_view name, std::array<const FunctionInstance*, N> instances)
        : FunctionGroup(name, insts) {
        std::copy_n(instances.data(), N, insts);
    }
};
constexpr const FunctionInstance* asFunctionInstance(const Function* func) {
    return (const FunctionInstance*)func;
}
constexpr const FunctionGroup* asFunctionGroup(const Function* func) {
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
    std::array<const Expr*, N> argumentStorage;
    constexpr UnresolvedCall(const Function* function, std::array<const Expr*, N> args)
        : CallExpr(function), argumentStorage(args) {
        arguments.setArray(argumentStorage.data(), N);
    }
    constexpr UnresolvedCall(const UnresolvedCall& other)
        : CallExpr(other.function), argumentStorage(other.argumentStorage) {
        arguments.setArray(argumentStorage.data(), N);
    }
};

}

namespace shadergen::functions {

using FP = FunctionParameter;
template<int_t N>
using ScalarCall = wrapped_scalar::impl<UnresolvedCall<N>>;
template<int_t N>
using VectorCall = wrapped_vector::impl<UnresolvedCall<N>>;

constexpr BuiltinFunctionInstance texture2DFunc { "texture", { FP::fixed(typeFromCXX<code::Sampler2D>), FP::fixed(FloatType.vectorType(2)) }, FP::fixed(FloatType.vectorType(4)) };
constexpr BuiltinFunctionInstance textureCubeFunc { "texture", { FP::fixed(typeFromCXX<code::SamplerCube>), FP::fixed(FloatType.vectorType(3)) }, FP::fixed(FloatType.vectorType(4)) };
constexpr BuiltinFunctionInstance texture2DShadowFunc { "texture", { FP::fixed(typeFromCXX<code::Sampler2DShadow>), FP::fixed(FloatType.vectorType(2)), FP::fixed(&FloatType) }, FP::fixed(&FloatType) };
constexpr BuiltinFunctionInstance textureCubeShadowFunc { "texture", { FP::fixed(typeFromCXX<code::SamplerCubeShadow>), FP::fixed(FloatType.vectorType(3)), FP::fixed(&FloatType) }, FP::fixed(&FloatType) };
namespace impls {
    constexpr wrapped_vector::impl<UnresolvedCall<2>> texture(const wrapped_user<code::Sampler2D>& sampler, const wrapped_vector& coord) { return { { &texture2DFunc, { sampler.wrappedExpr, coord.wrappedExpr } } }; }
}

constexpr BuiltinFunctionInstance floorFunc { "floor", { FP::dynamic(&FloatType) }, FP::dynamic(&FloatType) };
constexpr BuiltinFunctionInstance allFunc { "all", { FP::dynamic(&BoolType) }, FP::fixed(&BoolType) };
namespace impls {
    constexpr wrapped_vector::impl<UnresolvedCall<1>> floor(const wrapped_vector& arg) { return { { &floorFunc, { arg.wrappedExpr } } }; }
    constexpr wrapped_scalar::impl<UnresolvedCall<1>> floor(const wrapped_scalar& arg) { return { { &floorFunc, { arg.wrappedExpr } } }; }
    constexpr wrapped_scalar::impl<UnresolvedCall<1>> all(const wrapped_untyped& arg) { return { { &allFunc, { arg.wrappedExpr } } }; }
}

constexpr BuiltinFunctionInstance lessThanEqualFuncI { "lessThanEqual", { FP::dynamic(&IntType), FP::dynamic(&IntType) }, FP::dynamic(&BoolType) };
constexpr BuiltinFunctionInstance lessThanEqualFuncF { "lessThanEqual", { FP::dynamic(&FloatType), FP::dynamic(&FloatType) }, FP::dynamic(&BoolType) };
constexpr BuiltinFunctionGroup<2> lessThanEqualFunc { "lessThanEqual", { &lessThanEqualFuncI, &lessThanEqualFuncF } };
constexpr BuiltinFunctionInstance lessThanFuncI { "lessThan", { FP::dynamic(&IntType), FP::dynamic(&IntType) }, FP::dynamic(&BoolType) };
constexpr BuiltinFunctionInstance lessThanFuncF { "lessThan", { FP::dynamic(&FloatType), FP::dynamic(&FloatType) }, FP::dynamic(&BoolType) };
constexpr BuiltinFunctionGroup<2> lessThanFunc { "lessThan", { &lessThanFuncI, &lessThanFuncF } };
constexpr BuiltinFunctionInstance equalFuncI { "equal", { FP::dynamic(&IntType), FP::dynamic(&IntType) }, FP::dynamic(&BoolType) };
constexpr BuiltinFunctionInstance equalFuncF { "equal", { FP::dynamic(&FloatType), FP::dynamic(&FloatType) }, FP::dynamic(&BoolType) };
constexpr BuiltinFunctionGroup<2> equalFunc { "equal", { &equalFuncI, &equalFuncF } };
constexpr BuiltinFunctionInstance greaterThanEqualFuncI { "greaterThanEqual", { FP::dynamic(&IntType), FP::dynamic(&IntType) }, FP::dynamic(&BoolType) };
constexpr BuiltinFunctionInstance greaterThanEqualFuncF { "greaterThanEqual", { FP::dynamic(&FloatType), FP::dynamic(&FloatType) }, FP::dynamic(&BoolType) };
constexpr BuiltinFunctionGroup<2> greaterThanEqualFunc { "greaterThanEqual", { &greaterThanEqualFuncI, &greaterThanEqualFuncF } };
constexpr BuiltinFunctionInstance greaterThanFuncI { "greaterThan", { FP::dynamic(&IntType), FP::dynamic(&IntType) }, FP::dynamic(&BoolType) };
constexpr BuiltinFunctionInstance greaterThanFuncF { "greaterThan", { FP::dynamic(&FloatType), FP::dynamic(&FloatType) }, FP::dynamic(&BoolType) };
constexpr BuiltinFunctionGroup<2> greaterThanFunc { "greaterThan", { &greaterThanFuncI, &greaterThanFuncF } };
constexpr BuiltinFunctionInstance notEqualFuncI { "notEqual", { FP::dynamic(&IntType), FP::dynamic(&IntType) }, FP::dynamic(&BoolType) };
constexpr BuiltinFunctionInstance notEqualFuncF { "notEqual", { FP::dynamic(&FloatType), FP::dynamic(&FloatType) }, FP::dynamic(&BoolType) };
constexpr BuiltinFunctionGroup<2> notEqualFunc { "notEqual", { &notEqualFuncI, &notEqualFuncF } };
constexpr BuiltinFunctionInstance minFuncI { "min", { FP::dynamic(&IntType), FP::dynamic(&IntType) }, FP::dynamic(&IntType) };
constexpr BuiltinFunctionInstance minFuncF { "min", { FP::dynamic(&FloatType), FP::dynamic(&FloatType) }, FP::dynamic(&FloatType) };
constexpr BuiltinFunctionGroup<2> minFunc { "min", { &minFuncI, &minFuncF } };
constexpr BuiltinFunctionInstance maxFuncI { "max", { FP::dynamic(&IntType), FP::dynamic(&IntType) }, FP::dynamic(&IntType) };
constexpr BuiltinFunctionInstance maxFuncF { "max", { FP::dynamic(&FloatType), FP::dynamic(&FloatType) }, FP::dynamic(&FloatType) };
constexpr BuiltinFunctionGroup<2> maxFunc { "max", { &maxFuncI, &maxFuncF } };
namespace impls {
    constexpr wrapped_scalar::impl<UnresolvedCall<2>> lessThanEqual(const wrapped_scalar& l, const wrapped_scalar& r) { return { { &lessThanEqualFunc, { l.wrappedExpr, r.wrappedExpr } } }; }
    constexpr wrapped_vector::impl<UnresolvedCall<2>> lessThanEqual(const wrapped_vector& l, const wrapped_vector& r) { return { { &lessThanEqualFunc, { l.wrappedExpr, r.wrappedExpr } } }; }
    constexpr wrapped_scalar::impl<UnresolvedCall<2>> lessThan(const wrapped_scalar& l, const wrapped_scalar& r) { return { { &lessThanFunc, { l.wrappedExpr, r.wrappedExpr } } }; }
    constexpr wrapped_vector::impl<UnresolvedCall<2>> lessThan(const wrapped_vector& l, const wrapped_vector& r) { return { { &lessThanFunc, { l.wrappedExpr, r.wrappedExpr } } }; }
    constexpr wrapped_scalar::impl<UnresolvedCall<2>> equal(const wrapped_scalar& l, const wrapped_scalar& r) { return { { &equalFunc, { l.wrappedExpr, r.wrappedExpr } } }; }
    constexpr wrapped_vector::impl<UnresolvedCall<2>> equal(const wrapped_vector& l, const wrapped_vector& r) { return { { &equalFunc, { l.wrappedExpr, r.wrappedExpr } } }; }
    constexpr wrapped_scalar::impl<UnresolvedCall<2>> greaterThanEqual(const wrapped_scalar& l, const wrapped_scalar& r) { return { { &greaterThanEqualFunc, { l.wrappedExpr, r.wrappedExpr } } }; }
    constexpr wrapped_vector::impl<UnresolvedCall<2>> greaterThanEqual(const wrapped_vector& l, const wrapped_vector& r) { return { { &greaterThanEqualFunc, { l.wrappedExpr, r.wrappedExpr } } }; }
    constexpr wrapped_scalar::impl<UnresolvedCall<2>> greaterThan(const wrapped_scalar& l, const wrapped_scalar& r) { return { { &greaterThanFunc, { l.wrappedExpr, r.wrappedExpr } } }; }
    constexpr wrapped_vector::impl<UnresolvedCall<2>> greaterThan(const wrapped_vector& l, const wrapped_vector& r) { return { { &greaterThanFunc, { l.wrappedExpr, r.wrappedExpr } } }; }
    constexpr wrapped_scalar::impl<UnresolvedCall<2>> notEqual(const wrapped_scalar& l, const wrapped_scalar& r) { return { { &notEqualFunc, { l.wrappedExpr, r.wrappedExpr } } }; }
    constexpr wrapped_vector::impl<UnresolvedCall<2>> notEqual(const wrapped_vector& l, const wrapped_vector& r) { return { { &notEqualFunc, { l.wrappedExpr, r.wrappedExpr } } }; }
    constexpr wrapped_scalar::impl<UnresolvedCall<2>> min(const wrapped_scalar& l, const wrapped_scalar& r) { return { { &minFunc, { l.wrappedExpr, r.wrappedExpr } } }; }
    constexpr wrapped_vector::impl<UnresolvedCall<2>> min(const wrapped_vector& l, const wrapped_vector& r) { return { { &minFunc, { l.wrappedExpr, r.wrappedExpr } } }; }
    constexpr wrapped_scalar::impl<UnresolvedCall<2>> max(const wrapped_scalar& l, const wrapped_scalar& r) { return { { &maxFunc, { l.wrappedExpr, r.wrappedExpr } } }; }
    constexpr wrapped_vector::impl<UnresolvedCall<2>> max(const wrapped_vector& l, const wrapped_vector& r) { return { { &maxFunc, { l.wrappedExpr, r.wrappedExpr } } }; }
}

constexpr BuiltinFunctionInstance andFunc { "and", { FP::dynamic(&BoolType), FP::dynamic(&BoolType) }, FP::dynamic(&BoolType) };
constexpr BuiltinFunctionInstance orFunc { "or", { FP::dynamic(&BoolType), FP::dynamic(&BoolType) }, FP::dynamic(&BoolType) };
constexpr BuiltinFunctionInstance notFunc { "not", { FP::dynamic(&BoolType) }, FP::dynamic(&BoolType) };

}

namespace shadergen::code {
using namespace shadergen::functions::impls;
}
