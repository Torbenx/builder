#include "events/ExecutionContext.hpp"

void ExecutionContext::yieldTo(ExecutionContext* context) {
    asm volatile(
        "mov %%rsp, (%[old_ptr]);"
        "mov (%[new_ptr]), %%rsp;"
        :
        : [old_ptr] "a"(&stackPtr), [new_ptr] "b"(&context->stackPtr)
        : "rcx", "rdx", "rsi", "rdi", "rbp",
        "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
        "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7");
}

/*extern "C" void scheduler_schedule() {
    while (true) {
        if (tasks.empty()) {
            LOGD("exiting...");
            while (true) { }
        }
        Task* task = tasks.front();
        tasks.pop_front();
        if (!task->stack_pointer) {
            const int stackSize = 4096;
            task->stack = new void*[stackSize];
            task->stack_pointer = task->stack + stackSize / 2;
            asm volatile("mov %[stack_pointer], %%rsp;"
                         : "+r"(task), [stack_pointer] "+r"(task->stack_pointer)::);
            task->run();
        } else {
            asm volatile(
                "mov %[stack_pointer], %%rsp;"
                "jmp scheduler_task_continue;"
                : [stack_pointer] "+r"(task->stack_pointer)
                :
                :);
        }
    }
}*/
