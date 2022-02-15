#pragma once

#include "util/types.hpp"
#include <algorithm>
#include <limits>
#include <type_traits>

// fixed_string
template<int_t N>
struct fixed_string {
    char storage[N + 1] = {};

    constexpr fixed_string() { }
    constexpr fixed_string(const char (&str)[N + 1]) {
        std::copy(str, str + length() + 1, storage);
    }

    constexpr const char* data() const { return storage; }
    static constexpr int_t length() { return N; }
    constexpr std::string_view view() const { return { data(), length() }; }

    template<int_t M>
    constexpr bool operator==(const fixed_string<M>& other) const {
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
    constexpr auto operator+(const fixed_string<M>& other) const {
        fixed_string<N + M> ret;
        std::copy(data(), data() + length(), ret.storage);
        std::copy(other.data(), other.data() + other.length(), ret.storage + length());
        return ret;
    }
    template<int_t M>
    constexpr auto operator+(const char (&str)[M]) {
        return *this + fixed_string<M - 1>(str);
    }
};
template<int_t N>
fixed_string(const char (&)[N]) -> fixed_string<N - 1>;

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
template<typename B, typename T>
struct FieldInfo {
    int_t index = -1;
    fixed_string name = "";
    T B::*memberPtr = nullptr;
};
template<typename T, template<typename, typename, int_t, fixed_string, auto> typename Field, typename parent>
struct ref_fields_or_empty_t {
    struct type { };
    static constexpr bool IS_REFLECTED_TYPE = false;
};
template<typename T, template<typename, typename, int_t, fixed_string, auto> typename Field, typename parent>
requires requires { typename T::template _ref_store_<Field, parent>; }
struct ref_fields_or_empty_t<T, Field, parent> {
    using type = typename T::template _ref_store_<Field, parent>;
    static constexpr bool IS_REFLECTED_TYPE = true;
};
template<typename T, template<typename, typename, int_t, fixed_string, auto> typename Field, typename parent>
using ref_fields_or_empty = typename ref_fields_or_empty_t<T, Field, parent>::type;

template<typename parent_, typename type_, int_t index_, fixed_string name_, auto ptr>
struct RefField : ref_fields_or_empty<type_, RefField, RefField<parent_, type_, index_, name_, ptr>> {
    using Type = type_;

    static constexpr bool isField() { return ptr != nullptr; }
    static constexpr bool hasParent() { return !std::is_void_v<parent_>; }
    static constexpr bool hasReflectedType() { return ref_fields_or_empty_t<Type, RefField, parent_>::IS_REFLECTED_TYPE; }
    static constexpr auto parent() requires(hasParent()) { return parent_(); }
    static constexpr auto self() { return RefField(); }

    template<int_t I>
    static constexpr auto get() requires(hasReflectedType()) {
        return typename Type::template _ref_fields_<I, RefField, RefField>::current_field();
    }
    static constexpr int_t fieldCount() requires(hasReflectedType()) { return Type::_ref_field_count_; }
    static constexpr int_t leafFieldCount() requires(hasReflectedType()) { return Type::_ref_leaf_field_count_; }
    static constexpr int_t sizeInBytes() { return sizeof(Type); }
    static constexpr size_t alignment() { return alignof(Type); }
    template<typename Callback, int_t... I>
    static constexpr void forEachFieldBefore_helper(Callback&& callback, std::integer_sequence<int_t, I...>) requires(hasReflectedType()) {
        (callback(get<I>()), ...);
    }
    template<int_t I, typename Callback>
    static constexpr void forEachFieldBefore(Callback&& callback) {
        forEachFieldBefore_helper(std::forward<Callback>(callback), std::make_integer_sequence<int_t, I>());
    }
    template<typename Callback>
    static constexpr void forEachField(Callback&& callback) {
        forEachFieldBefore<fieldCount()>(std::forward<Callback>(callback));
    }
    static constexpr int_t indexInParent() { return index_; }
    static constexpr int_t leafIndexInParent() requires(hasParent()) {
        int_t leafIndex = 0;
        parent().template forEachFieldBefore<indexInParent()>([&](auto field) {
            if constexpr (field.hasReflectedType())
                leafIndex += field.leafFieldCount();
            else
                leafIndex += 1;
        });
        return leafIndex;
    }

    static constexpr auto name() requires(isField()) { return name_; }
    static constexpr auto memberPtr() { return ptr; }
    static constexpr auto parentOrRoot() {
        if constexpr (hasParent()) {
            return parent();
        } else {
            return RefField<void, MemberPointerBase<ptr>, -1, "", nullptr>();
        }
    }
    static constexpr int_t offsetInParent() { return OffsetOf<memberPtr()>; }

    template<typename Callback>
    static constexpr auto traverseTopDown(Callback&& callback) requires(hasParent()) {
        if constexpr (parent().hasParent()) {
            return callback(self(), parent().template traverseTopDown(std::forward<Callback>(callback)));
        } else {
            return callback(self());
        }
    }

    template<typename par, typename T, int_t idx, fixed_string str, auto mp, typename Callback>
    static constexpr auto traverseDownFrom(RefField<par, T, idx, str, mp> field, Callback&& callback) {
        if constexpr (field.self() == parentOrRoot()) {
            return callback(self());
        } else if constexpr (hasParent()) {
            return callback(self(), parent().traverseDownFrom(field, std::forward<Callback>(callback)));
        } else {
            static_assert(field.self() == parentOrRoot() || hasParent(), "field not found");
        }
    }

    template<typename par, typename T, int_t idx, fixed_string str, auto mp, typename Callback>
    static constexpr auto traverseUpTo_helper(RefField<par, T, idx, str, mp> field, Callback&& callback, auto value) {
        if constexpr (field.self() == parentOrRoot()) {
            return callback(self(), value);
        } else if constexpr (hasParent()) {
            return parent().traverseUpTo_helper(field, std::forward<Callback>(callback), callback(self(), value));
        } else {
            static_assert(field.self() == parentOrRoot() || hasParent(), "field not found");
        }
    }
    template<typename par, typename T, int_t idx, fixed_string str, auto mp, typename Callback>
    static constexpr auto traverseUpTo(RefField<par, T, idx, str, mp> field, Callback&& callback) {
        static_assert(hasParent());
        return parent().traverseUpTo_helper(field, std::forward<Callback>(callback), callback(self()));
    }

    template<typename par, typename T, int_t idx, fixed_string str, auto mp, typename Callback>
    static constexpr auto traverseDownTo(RefField<par, T, idx, str, mp> field, Callback&& callback) {
        return field.traverseDownFrom(self(), std::forward<Callback>(callback));
    }
    template<typename par, typename T, int_t idx, fixed_string str, auto mp, typename Callback>
    static constexpr auto traverseUpFrom(RefField<par, T, idx, str, mp> field, Callback&& callback) {
        return field.traverseUpTo(self(), std::forward<Callback>(callback));
    }

    template<fixed_string separator>
    static constexpr auto NAME_LAMBDA = []<int_t N = 0>(auto node, fixed_string<N> str = "") {
        if constexpr (str.empty())
            return node.name();
        else
            return str + separator + node.name();
    };
    template<fixed_string separator = ".", typename par, typename T, int_t idx, fixed_string str, auto mp>
    static constexpr auto nameIn(RefField<par, T, idx, str, mp> field) {
        return traverseDownFrom(field, NAME_LAMBDA<separator>);
    }
    template<typename par, typename T, int_t idx, fixed_string str, auto mp>
    static constexpr auto nameOf(RefField<par, T, idx, str, mp> field) {
        return field.nameIn(self());
    }
    template<fixed_string separator = ".">
    static constexpr auto fullName() {
        return traverseTopDown(NAME_LAMBDA<separator>);
    }

    static constexpr auto OFFSET_LAMBDA = [](auto node, int_t offset = 0) {
        return offset + node.offsetInParent();
    };
    template<typename par, typename T, int_t idx, fixed_string str, auto mp>
    static constexpr int_t offsetIn(RefField<par, T, idx, str, mp> field) {
        return traverseDownFrom(field, OFFSET_LAMBDA);
    }
    template<typename par, typename T, int_t idx, fixed_string str, auto mp>
    static constexpr auto offsetOf(RefField<par, T, idx, str, mp> field) {
        return field.offsetIn(self());
    }
    static constexpr int_t fullOffset() {
        return traverseTopDown(OFFSET_LAMBDA);
    }

    static constexpr auto LEAF_INDEX_LAMBDA = [](auto node, int_t leafIndex = 0) {
        return leafIndex + node.leafIndexInParent();
    };
    template<typename par, typename T, int_t idx, fixed_string str, auto mp>
    static constexpr int_t leafIndexIn(RefField<par, T, idx, str, mp> field) {
        return traverseDownFrom(field, LEAF_INDEX_LAMBDA);
    }
    template<typename par, typename T, int_t idx, fixed_string str, auto mp>
    static constexpr int_t leafIndexOf(RefField<par, T, idx, str, mp> field) {
        return field.leafIndexIn(self());
    }
    static constexpr int_t fullLeafIndex() {
        return traverseTopDown(LEAF_INDEX_LAMBDA);
    }

    template<typename T>
    static constexpr auto& access(T& root) {
        return *traverseTopDown([&]<typename S = std::nullptr_t>(auto node, S element = {}) {
            if constexpr (std::is_same_v<decltype(element), std::nullptr_t>)
                return &(root.*node.memberPtr());
            else
                return &(element->*node.memberPtr());
        });
    }

    constexpr bool operator==(const RefField&) const { return true; }
    template<typename par, typename T, int_t idx, fixed_string str, auto mp>
    constexpr bool operator==(const RefField<par, T, idx, str, mp>&) const { return false; }
    template<typename T>
    constexpr auto& operator()(T& root) {
        return access(root);
    }
};

template<typename T>
struct reflect : RefField<void, T, -1, "", nullptr> {
    constexpr reflect() { }
    constexpr reflect(const T&) { }
};
template<typename T>
reflect(const T&) -> reflect<T>;

template<typename T>
static constexpr int_t computeLeafFieldCount() {
    auto r = reflect<T>();
    int_t count = 0;
    r.forEachField([&](auto field) {
        if constexpr (field.hasReflectedType())
            count += field.leafFieldCount();
        else
            count += 1;
    });
    return count;
}

#define BEGIN_REF_STRUCT(base)                                                                                           \
    template<int_t field_index, template<typename, typename, int_t, fixed_string, auto> typename Field, typename parent> \
    struct _ref_fields_;                                                                                                 \
    template<template<typename, typename, int_t, fixed_string, auto> typename Field, typename parent>                    \
    struct _ref_fields_<-1, Field, parent> { };                                                                          \
    using _ref_base_alias_ = base;                                                                                       \
    static constexpr int_t _ref_counter_offset_ = __COUNTER__ + 1;

#define REF_FIELD_HELPER(name, counter)                                                                                                        \
    name;                                                                                                                                      \
    template<template<typename, typename, int_t, fixed_string, auto> typename Field, typename parent>                                          \
    struct _ref_fields_<counter - _ref_counter_offset_, Field, parent> : _ref_fields_<counter - _ref_counter_offset_ - 1, Field, parent> {     \
        using current_field = Field<parent, decltype(_ref_base_alias_::name), counter - _ref_counter_offset_, #name, &_ref_base_alias_::name>; \
        current_field name;                                                                                                                    \
    };

#define REF_FIELD(name) REF_FIELD_HELPER(name, __COUNTER__)

#define END_REF_STRUCT()                                                                              \
    static constexpr int_t _ref_field_count_ = __COUNTER__ - _ref_counter_offset_;                    \
    template<template<typename, typename, int_t, fixed_string, auto> typename Field, typename parent> \
    using _ref_store_ = _ref_fields_<_ref_field_count_ - 1, Field, parent>;                           \
    static constexpr int_t _ref_leaf_field_count_ = computeLeafFieldCount<_ref_base_alias_>();
