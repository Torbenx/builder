#pragma once

#include "util/log.hpp"
#include "util/target.hpp"

template<typename Callback>
void ExecutionContext_bootstrap_callback(void* data) {
    Callback* callback_ptr = (Callback*)data;
    (*callback_ptr)();
}

class ExecutionContext {
public:
    ExecutionContext() { }
    ExecutionContext(const ExecutionContext&) = delete;
    void yieldTo(ExecutionContext* context);
    void bootstrap(void (*callback)(void*), void* stack_pointer, void* data) {
        bool should_exit = false;
        bootstrap_interal(callback, stack_pointer, data, &should_exit);
    }

private:
    void* stackPtr = nullptr;
    void bootstrap_interal(void (*callback)(void*), void* stack_pointer, void* data, bool*);
};
