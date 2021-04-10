#include "events/Events.hpp"
#include "events/ExecutionContext.hpp"
#include <list>

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

std::list<Task*> tasks;
void** scheduler_stack_pointer;

struct ATask : public Task {
    virtual void run() override {
        for (int i = 0; i < 10; i++) {
            LOGD("a: %d", i);
            tasks.push_back(this);
            yield();
        }
    }
};

struct BTask : public Task {
    virtual void run() override {
        for (int i = 0; i < 15; i++) {
            LOGD("b: %d", i);
            tasks.push_back(this);
            yield();
        }
    }
};

extern "C" void scheduler_schedule() {
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
            asm volatile(
                "mov %[stack_pointer], %%rsp;"
                : "+r"(task), [stack_pointer] "+r"(task->stack_pointer)
                :
                :);
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
}

void Scheduler::execute(Task* task) {
    int stackSize = 4096;
    void** stack = new void*[stackSize];
    scheduler_stack_pointer = stack + stackSize;
    tasks.push_back(new ATask());
    tasks.push_back(new BTask());
    // tasks.push_back(task);
    task->yield();
}

void Task::yield() {
    asm volatile(
        "mov %%rsp, (%[stack_pointer]);"
        "mov %[scheduler_stack_pointer], %%rsp;"
        "jmp scheduler_schedule;"
        "scheduler_task_continue:"
        :
        : [stack_pointer] "a"(&(this->stack_pointer)), [scheduler_stack_pointer] "b"(scheduler_stack_pointer)
        : "rcx", "rdx", "rsi", "rdi", "rbp", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15", "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7");
}
