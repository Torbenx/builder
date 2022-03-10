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
    constexpr FunctionGroup(std::string_view name, ArrayView<const FunctionInstance*> instances)
        : Function(true, name), instances(instances) { }
};
template<int_t N>
struct BuiltinFunctionGroup : FunctionGroup {
    const FunctionInstance* insts[N];
    constexpr BuiltinFunctionGroup(std::string_view name, const FunctionInstance* const (&instances)[N])
        : FunctionGroup(name, insts) {
        std::copy_n(instances, N, insts);
    }
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

static constexpr BuiltinFunctionInstance lessThanEqualFuncI { "lessThanEqual", { FunctionParameter::makeDynamic(&IntType), FunctionParameter::makeDynamic(&IntType) }, FunctionParameter::makeDynamic(&BoolType) };
static constexpr BuiltinFunctionInstance lessThanEqualFuncF { "lessThanEqual", { FunctionParameter::makeDynamic(&FloatType), FunctionParameter::makeDynamic(&FloatType) }, FunctionParameter::makeDynamic(&BoolType) };
static constexpr BuiltinFunctionGroup lessThanEqualFunc { "lessThanEqual", { &lessThanEqualFuncI, &lessThanEqualFuncF } };
static constexpr BuiltinFunctionInstance lessThanFuncI { "lessThan", { FunctionParameter::makeDynamic(&IntType), FunctionParameter::makeDynamic(&IntType) }, FunctionParameter::makeDynamic(&BoolType) };
static constexpr BuiltinFunctionInstance lessThanFuncF { "lessThan", { FunctionParameter::makeDynamic(&FloatType), FunctionParameter::makeDynamic(&FloatType) }, FunctionParameter::makeDynamic(&BoolType) };
static constexpr BuiltinFunctionGroup lessThanFunc { "lessThan", { &lessThanFuncI, &lessThanFuncF } };
static constexpr BuiltinFunctionInstance equalFuncI { "equal", { FunctionParameter::makeDynamic(&IntType), FunctionParameter::makeDynamic(&IntType) }, FunctionParameter::makeDynamic(&BoolType) };
static constexpr BuiltinFunctionInstance equalFuncF { "equal", { FunctionParameter::makeDynamic(&FloatType), FunctionParameter::makeDynamic(&FloatType) }, FunctionParameter::makeDynamic(&BoolType) };
static constexpr BuiltinFunctionGroup equalFunc { "equal", { &equalFuncI, &equalFuncF } };
static constexpr BuiltinFunctionInstance greaterThanEqualFuncI { "greaterThanEqual", { FunctionParameter::makeDynamic(&IntType), FunctionParameter::makeDynamic(&IntType) }, FunctionParameter::makeDynamic(&BoolType) };
static constexpr BuiltinFunctionInstance greaterThanEqualFuncF { "greaterThanEqual", { FunctionParameter::makeDynamic(&FloatType), FunctionParameter::makeDynamic(&FloatType) }, FunctionParameter::makeDynamic(&BoolType) };
static constexpr BuiltinFunctionGroup greaterThanEqualFunc { "greaterThanEqual", { &greaterThanEqualFuncI, &greaterThanEqualFuncF } };
static constexpr BuiltinFunctionInstance greaterThanFuncI { "greaterThan", { FunctionParameter::makeDynamic(&IntType), FunctionParameter::makeDynamic(&IntType) }, FunctionParameter::makeDynamic(&BoolType) };
static constexpr BuiltinFunctionInstance greaterThanFuncF { "greaterThan", { FunctionParameter::makeDynamic(&FloatType), FunctionParameter::makeDynamic(&FloatType) }, FunctionParameter::makeDynamic(&BoolType) };
static constexpr BuiltinFunctionGroup greaterThanFunc { "greaterThan", { &greaterThanFuncI, &greaterThanFuncF } };
static constexpr BuiltinFunctionInstance notEqualFuncI { "notEqual", { FunctionParameter::makeDynamic(&IntType), FunctionParameter::makeDynamic(&IntType) }, FunctionParameter::makeDynamic(&BoolType) };
static constexpr BuiltinFunctionInstance notEqualFuncF { "notEqual", { FunctionParameter::makeDynamic(&FloatType), FunctionParameter::makeDynamic(&FloatType) }, FunctionParameter::makeDynamic(&BoolType) };
static constexpr BuiltinFunctionGroup notEqualFunc { "notEqual", { &notEqualFuncI, &notEqualFuncF } };

static constexpr BuiltinFunctionInstance andFunc { "and", { FunctionParameter::makeDynamic(&BoolType), FunctionParameter::makeDynamic(&BoolType) }, FunctionParameter::makeDynamic(&BoolType) };
static constexpr BuiltinFunctionInstance orFunc { "or", { FunctionParameter::makeDynamic(&BoolType), FunctionParameter::makeDynamic(&BoolType) }, FunctionParameter::makeDynamic(&BoolType) };
static constexpr BuiltinFunctionInstance notFunc { "not", { FunctionParameter::makeDynamic(&BoolType) }, FunctionParameter::makeDynamic(&BoolType) };

namespace code {
    constexpr wrapped_vector::impl<UnresolvedCall<1>> floor(const wrapped_vector& arg) { return { { &floorFunc, { arg.wrappedExpr } } }; }
    constexpr wrapped_scalar::impl<UnresolvedCall<1>> floor(const wrapped_scalar& arg) { return { { &floorFunc, { arg.wrappedExpr } } }; }
    constexpr wrapped_scalar::impl<UnresolvedCall<1>> all(const wrapped_untyped& arg) { return { { &allFunc, { arg.wrappedExpr } } }; }

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
}

}
