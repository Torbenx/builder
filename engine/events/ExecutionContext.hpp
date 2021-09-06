#pragma once

#include "util/target.hpp"
#include <cstddef>

#if __has_include(<coroutine>)

#include <coroutine>
namespace stdx = std;

#else

#include <experimental/coroutine>
namespace stdx = std::experimental;

#endif

#ifdef PLATFORM_WINDOWS
#define MIGHT_YIELD __attribute__((sysv_abi))
#else
#define MIGHT_YIELD
#endif

class ExecutionContext {
public:
    ExecutionContext() { }
    ExecutionContext(const ExecutionContext&) = delete;
    void MIGHT_YIELD yieldTo(ExecutionContext* context);
    void MIGHT_YIELD bootstrap(void(MIGHT_YIELD* callback)(void*), void* stack_pointer, void* data) {
        bool should_exit = false;
        bootstrap_interal(callback, stack_pointer, data, &should_exit);
    }
    bool isValid() {
        return stackPtr != nullptr;
    }

private:
    void* stackPtr = nullptr;
    void MIGHT_YIELD bootstrap_interal(void(MIGHT_YIELD* callback)(void*), void* stack_pointer, void* data, bool*);
};
