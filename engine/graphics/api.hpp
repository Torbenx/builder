#pragma once

#include <variant>

namespace gfx {

enum class Api {
    Invalid,
    Software,
    OpenGL,
    Vulkan,
};

template<template<Api> typename Data>
struct Variant : std::variant<std::monostate, Data<(Api)1>, Data<(Api)2>, Data<(Api)3>> {
    using Base = std::variant<std::monostate, Data<(Api)1>, Data<(Api)2>, Data<(Api)3>>;
    using Base::Base;
    using Base::operator=;
    Api api() const { return (Api)this->index(); }
    void reset() { this->template emplace<0>(); }
};

template<Api api, template<Api> typename Data>
auto& get(Variant<Data>& var) {
    return std::get<(std::size_t)api>(var);
}
template<Api api, template<Api> typename Data>
const auto& get(const Variant<Data>& var) {
    return std::get<(std::size_t)api>(var);
}

}

#define MAKE_API_TEMPLATE_PREFIXED(Name)       \
    template<gfx::Api>                         \
    struct Name##_helper;                      \
    template<>                                 \
    struct Name##_helper<gfx::Api::Software> { \
        using type = SW##Name;                 \
    };                                         \
    template<>                                 \
    struct Name##_helper<gfx::Api::OpenGL> {   \
        using type = GL##Name;                 \
    };                                         \
    template<>                                 \
    struct Name##_helper<gfx::Api::Vulkan> {   \
        using type = VK##Name;                 \
    };                                         \
    template<gfx::Api api>                     \
    using Name = typename Name##_helper<api>::type;
