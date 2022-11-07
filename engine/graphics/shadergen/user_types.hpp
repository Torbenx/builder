#pragma once

#include "vectors.hpp"

namespace shadergen {

struct MemberInfo {
    std::string_view name;
    const Type* type;
    int_t offset;
    int_t leafFieldOffset;
};
struct UserType : NonArrayType {
    std::string_view name;
    ArrayView<const MemberInfo> members;
    int_t leafFieldCount = 0;
    bool builtin;
    constexpr UserType(std::string_view name, bool builtin = false)
        : NonArrayType(true, false, false, false, false), name(name), builtin(builtin) { }
};
constexpr const UserType* asUserType(const Type* type) {
    VERIFY(ShaderGen, type->isUser);
    return static_cast<const UserType*>(type);
}

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

    template<typename T>
    static constexpr FieldInfo create(std::string_view name) { return { name, typeFromCXX<T>, alignof(T), sizeof(T) }; }
};

// makeUserType
constexpr int_t getLeafFieldCount(const Type* type) {
    if (type->isMatrix) {
        const auto* mType = asMatrixType(type);
        return mType->vectorSize * mType->covectorSize;
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
        size_t offset = stx::alignmentCeil(alignment, prevEnd);
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
constexpr MemberInfo makeMemberInfo(FieldInfo info, StructLayoutHelper& layout, LeafFieldHelper& leafFields) {
    return { info.name, info.type, (int_t)layout.addField(info.alignment, info.size), leafFields.addField(info.type) };
}
template<int_t N>
struct UserTypeFieldInfos {
    stx::Array<MemberInfo, N> members;
    int_t leafFieldCount = 0;
};
template<typename T, int_t... I>
constexpr auto makeUserTypeFieldInfos(stx::int_sequence<I...>) {
    UserTypeFieldInfos<sizeof...(I)> out;
    StructLayoutHelper layout;
    LeafFieldHelper leafFields;
    ((out.members[I] = makeMemberInfo(T::template _gen_field_holder_<I>::value, layout, leafFields)), ...);
    out.leafFieldCount = leafFields.leafFieldCount;
    return out;
}
template<typename T, int_t N>
inline constexpr UserTypeFieldInfos<N> userTypeFieldInfos = makeUserTypeFieldInfos<T>(stx::make_int_sequence<N>());
template<typename T, int_t N>
struct UserTypeImpl : UserType {
    constexpr UserTypeImpl()
        : UserType(T::_gen_struct_name_, false) {
        this->members = userTypeFieldInfos<T, N>.members;
        this->leafFieldCount = userTypeFieldInfos<T, N>.leafFieldCount;
    }
};

// macros
#define BEGIN_GEN_STRUCT(name)                                           \
    template<int_t fieldIndex, typename T = void>                        \
    struct _gen_fields_;                                                 \
    template<typename T>                                                 \
    struct _gen_fields_<-1, T> : ::shadergen::wrapped_user_untyped { };  \
    using _gen_base_alias_ = name;                                       \
    template<int_t fieldIndex, typename T = void>                        \
    struct _gen_field_holder_;                                           \
    static constexpr int_t _gen_field_counter_offset_ = __COUNTER__ + 1; \
    static constexpr std::string_view _gen_struct_name_ = #name;

#define GEN_FIELD_HELPER(name, index)                                                                                                          \
    name;                                                                                                                                      \
    template<typename T>                                                                                                                       \
    struct _gen_fields_<index, T> : _gen_fields_<index - 1, T> {                                                                               \
        ::shadergen::get_wrapper<decltype(_gen_base_alias_::name)>::impl<::shadergen::MemberAccessExpr> name { { this->wrappedExpr, index } }; \
    };                                                                                                                                         \
    template<typename T>                                                                                                                       \
    struct _gen_field_holder_<index, T> {                                                                                                      \
        static constexpr ::shadergen::FieldInfo value = ::shadergen::FieldInfo::create<decltype(_gen_base_alias_::name)>(#name);               \
    }

#define GEN_FIELD(name) GEN_FIELD_HELPER(name, __COUNTER__ - _gen_field_counter_offset_);

#define END_GEN_STRUCT_HELPER(index)                    \
    using _gen_fields_final_ = _gen_fields_<index - 1>; \
    static constexpr ::shadergen::UserTypeImpl<_gen_base_alias_, index> SHADERGEN_TYPE {};

#define END_GEN_STRUCT() END_GEN_STRUCT_HELPER(__COUNTER__ - _gen_field_counter_offset_);

// wrappers
struct wrapped_user_untyped {
    const Expr* wrappedExpr;
};
template<typename T>
struct wrapped_user : T::_gen_fields_final_ {
    template<typename E>
    struct impl;
};
template<typename T>
template<typename E>
struct wrapped_user<T>::impl : E, wrapped_user {
    using wrapper_base = wrapped_user;
    constexpr impl(const E& e)
        : E(e), wrapped_user { { { (Expr*)this } } } { }
    constexpr AssignmentStmt operator=(const impl& right) const {
        return { this, right.wrappedExpr };
    }
    constexpr AssignmentStmt operator=(const wrapped_user& right) const {
        return { this, right.wrappedExpr };
    }
};

template<typename T>
using get_non_array_wrapper = std::conditional_t<
    typeFromCXX<T>->isScalar,
    wrapped_scalar,
    std::conditional_t<
        typeFromCXX<T>->isVector,
        wrapped_vector,
        std::conditional_t<typeFromCXX<T>->isMatrix, wrapped_matrix, wrapped_user<T>>>>;

template<typename T>
using get_wrapper = std::conditional_t<typeFromCXX<T>->isArray, wrapped_array<get_non_array_wrapper<std::remove_extent_t<T>>>, get_non_array_wrapper<T>>;

// opaque sampler types
struct BuiltinCXXType {
    struct _gen_fields_final_ : wrapped_user_untyped { };
};
struct BuiltinUserType : UserType {
    constexpr BuiltinUserType(std::string_view name)
        : UserType(name, true) { }
};
namespace code {
    struct Sampler2D : BuiltinCXXType {
        static constexpr BuiltinUserType SHADERGEN_TYPE { "sampler2D" };
    };
    struct SamplerCube : BuiltinCXXType {
        static constexpr BuiltinUserType SHADERGEN_TYPE { "samplerCube" };
    };
    struct Sampler2DShadow : BuiltinCXXType {
        static constexpr BuiltinUserType SHADERGEN_TYPE { "sampler2DShadow" };
    };
    struct SamplerCubeShadow : BuiltinCXXType {
        static constexpr BuiltinUserType SHADERGEN_TYPE { "samplerCubeShadow" };
    };
    struct iImage2D : BuiltinCXXType {
        static constexpr BuiltinUserType SHADERGEN_TYPE { "iimage2D" };
    };
    struct uImage2D : BuiltinCXXType {
        static constexpr BuiltinUserType SHADERGEN_TYPE { "uimage2D" };
    };
    struct fImage2D : BuiltinCXXType {
        static constexpr BuiltinUserType SHADERGEN_TYPE { "fimage2D" };
    };
}

}
