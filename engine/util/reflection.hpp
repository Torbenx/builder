#pragma once

#include "util/types.hpp"
#include <algorithm>
#include <limits>
#include <type_traits>

// ConstexprString
template<int_t N>
struct ConstexprString {
    char storage[N + 1] = {};

    constexpr ConstexprString() { }
    constexpr ConstexprString(const char (&str)[N + 1]) {
        std::copy(str, str + length() + 1, storage);
    }

    constexpr const char* data() const { return storage; }
    static constexpr int_t length() { return N; }
    constexpr std::string_view view() const { return { data(), length() }; }

    template<int_t M>
    constexpr bool operator==(const ConstexprString<M>& other) const {
        if constexpr (N != M) {
            return false;
        } else {
            for (int_t i = 0; i < N + 1; i++) {
                if (data()[i] != other.data()[i])
                    return false;
            }
            return true;
        }
    }

    static constexpr bool empty() { return N == 0; }

    template<int_t M>
    constexpr auto operator+(const ConstexprString<M>& other) const {
        ConstexprString<N + M> ret;
        std::copy(data(), data() + length(), ret.storage);
        std::copy(other.data(), other.data() + other.length(), ret.storage + length());
        return ret;
    }
    template<int_t M>
    constexpr auto operator+(const char (&str)[M]) {
        return *this + ConstexprString<M - 1>(str);
    }
};
template<int_t N>
ConstexprString(const char (&)[N]) -> ConstexprString<N - 1>;

// member pointer utils
template<typename T>
struct member_pointer_helper;

template<typename T, typename B>
struct member_pointer_helper<T B::*> {
    using Type = T;
    using Base = B;
};

template<auto ptr>
struct member_pointer : member_pointer_helper<decltype(ptr)> { };

template<auto ptr>
using MemberPointerType = typename member_pointer<ptr>::Type;

template<auto ptr>
using MemberPointerBase = typename member_pointer<ptr>::Base;

template<auto ptr>
static constexpr int OffsetOf = __builtin_offsetof(MemberPointerBase<ptr>, __builtin_member_ptr[ptr]);

// reflection
template<typename T, template<typename, typename, ConstexprString, auto> typename Field, typename parent>
struct ref_store_or_empty_t {
    struct type { };
};
template<typename T, template<typename, typename, ConstexprString, auto> typename Field, typename parent>
requires requires { typename T::template _ref_store_<Field, parent>; }
struct ref_store_or_empty_t<T, Field, parent> {
    using type = typename T::template _ref_store_<Field, parent>;
};
template<typename T, template<typename, typename, ConstexprString, auto> typename Field, typename parent>
using ref_store_or_empty = typename ref_store_or_empty_t<T, Field, parent>::type;

template<typename parent_, typename type_, ConstexprString name_, auto ptr>
struct RefField : ref_store_or_empty<type_, RefField, RefField<parent_, type_, name_, ptr>> {
    using Type = type_;

    static constexpr bool isField() { return ptr != nullptr; }
    static constexpr bool hasParent() { return !std::is_void_v<parent_>; }
    static constexpr auto parent() requires(hasParent()) { return parent_(); }
    static constexpr auto self() { return RefField(); }

    template<int_t I>
    static constexpr auto get() {
        return typename Type::template _ref_struct_<I, RefField>::current_field();
    }
    static constexpr int_t fieldCount() { return Type::_ref_field_count_; }
    static constexpr int_t sizeInBytes() { return sizeof(Type); }
    static constexpr size_t alignment() { return alignof(Type); }

    static constexpr auto name() { return name_; }
    static constexpr auto memberPtr() { return ptr; }
    static constexpr auto parentOrRoot() {
        if constexpr (hasParent()) {
            return parent();
        } else {
            return RefField<void, MemberPointerBase<ptr>, "", nullptr>();
        }
    }
    static constexpr int_t offsetInParent() { return OffsetOf<memberPtr()>; }

    static constexpr int_t MAX_DEPTH = std::numeric_limits<int_t>::max();
    template<int_t maxDepth = MAX_DEPTH, typename Callback>
    static constexpr auto traverseTopDown(Callback&& callback) {
        if constexpr (maxDepth == 0 || !hasParent()) {
            return callback(self());
        } else {
            return callback(self(), parent().template traverseTopDown<maxDepth - 1>(std::forward<Callback>(callback)));
        }
    }

    template<typename par, typename T, ConstexprString str, auto mp, typename Callback>
    static constexpr auto traverseDownFrom(RefField<par, T, str, mp> field, Callback&& callback) {
        if constexpr (field.self() == parentOrRoot()) {
            return callback(self());
        } else if constexpr (hasParent()) {
            return callback(self(), parent().traverseDownFrom(field, std::forward<Callback>(callback)));
        } else {
            static_assert(field.self() == parentOrRoot() || hasParent(), "field not found");
        }
    }

    template<typename par, typename T, ConstexprString str, auto mp, typename Callback>
    static constexpr auto traverseUpTo_helper(RefField<par, T, str, mp> field, Callback&& callback, auto value) {
        if constexpr (field.self() == parentOrRoot()) {
            return callback(self(), value);
        } else if constexpr (hasParent()) {
            return parent().traverseUpTo_helper(field, std::forward<Callback>(callback), callback(self(), value));
        } else {
            static_assert(field.self() == parentOrRoot() || hasParent(), "field not found");
        }
    }
    template<typename par, typename T, ConstexprString str, auto mp, typename Callback>
    static constexpr auto traverseUpTo(RefField<par, T, str, mp> field, Callback&& callback) {
        static_assert(hasParent());
        return parent().traverseUpTo_helper(field, std::forward<Callback>(callback), callback(self()));
    }

    template<typename par, typename T, ConstexprString str, auto mp, typename Callback>
    static constexpr auto traverseDownTo(RefField<par, T, str, mp> field, Callback&& callback) {
        return field.traverseDownFrom(self(), std::forward<Callback>(callback));
    }
    template<typename par, typename T, ConstexprString str, auto mp, typename Callback>
    static constexpr auto traverseUpFrom(RefField<par, T, str, mp> field, Callback&& callback) {
        return field.traverseUpTo(self(), std::forward<Callback>(callback));
    }

    template<ConstexprString separator>
    static constexpr auto NAME_LAMBDA = []<int_t N = 0>(auto node, ConstexprString<N> str = "") {
        if constexpr (str.empty())
            return node.name();
        else
            return str + separator + node.name();
    };
    template<ConstexprString separator = ".", typename par, typename T, ConstexprString str, auto mp>
    static constexpr auto nameIn(RefField<par, T, str, mp> field) {
        return traverseDownFrom(field, NAME_LAMBDA<separator>);
    }
    template<typename par, typename T, ConstexprString str, auto mp>
    static constexpr auto nameOf(RefField<par, T, str, mp> field) {
        return field.nameIn(self());
    }
    template<ConstexprString separator = ".">
    static constexpr auto fullName() {
        return traverseTopDown(NAME_LAMBDA<separator>);
    }

    static constexpr auto OFFSET_LAMBDA = [](auto node, int_t offset = 0) {
        return offset + node.offsetInParent();
    };
    template<typename par, typename T, ConstexprString str, auto mp>
    static constexpr int_t offsetIn(RefField<par, T, str, mp> field) {
        return traverseDownFrom(field, OFFSET_LAMBDA);
    }
    template<typename par, typename T, ConstexprString str, auto mp>
    static constexpr auto offsetOf(RefField<par, T, str, mp> field) {
        return field.offsetIn(self());
    }
    static constexpr int_t fullOffset() {
        return traverseTopDown(OFFSET_LAMBDA);
    }

    template<typename T>
    static constexpr auto& access(T& root) {
        if constexpr (hasParent()) {
            return parent().access(root).*memberPtr();
        } else {
            static_assert(hasParent() || std::is_same_v<T, Type>);
            return root;
        }
    }

    constexpr bool operator==(const RefField&) const { return true; }
    template<typename par, typename T, ConstexprString str, auto mp>
    constexpr bool operator==(const RefField<par, T, str, mp>&) const { return false; }
    template<typename T>
    constexpr auto& operator()(T& root) {
        return access(root);
    }
};

template<typename T>
struct reflect : RefField<void, T, "", nullptr> {
    constexpr reflect() { }
    constexpr reflect(const T&) { }
};
template<typename T>
reflect(const T&) -> reflect<T>;

#define BEGIN_REF_STRUCT(base)                                                                                       \
    template<int_t field_index, template<typename, typename, ConstexprString, auto> typename Field, typename parent> \
    struct _ref_struct_;                                                                                             \
    template<template<typename, typename, ConstexprString, auto> typename Field, typename parent>                    \
    struct _ref_struct_<-1, Field, parent> { };                                                                      \
    using _ref_base_alias_ = base;                                                                                   \
    static constexpr int_t _ref_counter_offset_ = __COUNTER__ + 1;

#define REF_FIELD_HELPER(name, counter)                                                                                                    \
    name;                                                                                                                                  \
    template<template<typename, typename, ConstexprString, auto> typename Field, typename parent>                                          \
    struct _ref_struct_<counter - _ref_counter_offset_, Field, parent> : _ref_struct_<counter - _ref_counter_offset_ - 1, Field, parent> { \
        using current_field = Field<parent, decltype(_ref_base_alias_::name), #name, &_ref_base_alias_::name>;                             \
        current_field name;                                                                                                                \
    };

#define REF_FIELD(name) REF_FIELD_HELPER(name, __COUNTER__)

#define END_REF_STRUCT()                                                                          \
    static constexpr int_t _ref_field_count_ = __COUNTER__ - _ref_counter_offset_;                \
    template<template<typename, typename, ConstexprString, auto> typename Field, typename parent> \
    using _ref_store_ = _ref_struct_<_ref_field_count_ - 1, Field, parent>;
