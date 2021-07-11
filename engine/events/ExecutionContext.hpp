#pragma once

#include "util/log.hpp"
#include "util/target.hpp"

class ExecutionContext {
public:
    ExecutionContext() { }
    ExecutionContext(const ExecutionContext&) = delete;
    void __attribute__((sysv_abi)) yieldTo(ExecutionContext* context);
    void __attribute__((sysv_abi)) bootstrap(void (*callback)(void*), void* stack_pointer, void* data) {
        bool should_exit = false;
        bootstrap_interal(callback, stack_pointer, data, &should_exit);
    }
    bool isValid() {
        return stackPtr != nullptr;
    }

private:
    void* stackPtr = nullptr;
    void __attribute__((sysv_abi)) bootstrap_interal(void (*callback)(void*), void* stack_pointer, void* data, bool*);
};
