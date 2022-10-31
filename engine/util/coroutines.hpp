#pragma once

#include "util/log.hpp"

#if __has_include(<coroutine>)
#include <coroutine>
namespace stdx = std;
#else
#include <experimental/coroutine>
namespace stdx = std::experimental;
#endif

namespace stx {

template<typename T>
struct await_value {
    T t;
    bool await_ready() { return true; }
    bool await_suspend(stdx::coroutine_handle<>) { return false; }
    T await_resume() { return t; }
};

struct get_self_t { };
static constexpr get_self_t get_self = {};

struct VoidPromise {
    void get_return_object() { }
    std::suspend_never initial_suspend() { return {}; }
    std::suspend_never final_suspend() noexcept { return {}; }
    void return_void() { }
    void unhandled_exception() { VERIFY_NOT_REACHED(Default); }
};

}

template<typename T, typename... Args>
struct stdx::coroutine_traits<T*, Args...> {
    using promise_type = T;
};
template<typename T, typename... Args>
struct stdx::coroutine_traits<const T*, Args...> {
    using promise_type = T;
};
template<typename... Args>
struct stdx::coroutine_traits<void, Args...> {
    using promise_type = stx::VoidPromise;
};
