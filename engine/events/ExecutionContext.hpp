#pragma once

#include <functional>

class ExecutionContext {
public:
    ExecutionContext() { }
    ExecutionContext(const ExecutionContext&) = delete;
    void yieldTo(ExecutionContext* context);

    template<typename Callback>
    static void bootstrap(Callback callback) {
        ExecutionContext boot;
        boot.yieldTo(&boot);
        asm volatile("sub $0x100, %rsp;");
        callback(&boot);
        LOGD("ExecutionContext bootstrap should never return!");
        while (true) { }
    }

private:
    void* stackPtr = nullptr;
};
