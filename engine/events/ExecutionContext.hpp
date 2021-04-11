#pragma once

#include <functional>

class ExecutionContext {
public:
    ExecutionContext() { }
    ExecutionContext(const ExecutionContext&) = delete;
    void yieldTo(ExecutionContext* context);

    template<typename Callback>
    void bootstrap(Callback callback) {
        yieldTo(this);
        asm volatile("sub $0x100, %rsp;");
        callback();
        asm volatile("add $0x100, %rsp;");
    }

private:
    void* stackPtr = nullptr;
};
