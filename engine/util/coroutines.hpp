#pragma once

#if __has_include(<coroutine>)
#include <coroutine>
namespace stdx = std;
#else
#include <experimental/coroutine>
namespace stdx = std::experimental;
#endif

template<typename T>
struct await_value {
    T t;
    bool await_ready() { return true; }
    bool await_suspend(stdx::coroutine_handle<>) { return false; }
    T await_resume() { return t; }
};

struct get_self_t { };
static constexpr get_self_t get_self = {};