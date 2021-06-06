#pragma once

#include "events/ExecutionContext.hpp"
#include "util/AtomicList.hpp"
#include <atomic>

class Stack {
public:
    Stack() {
        size = 4096;
        baseAddr = new void*[size];
    }

    ~Stack() {
        delete baseAddr;
    }

    void* origin() {
        return baseAddr + size;
    }
    size_t sizeInBytes() {
        return size * sizeof(void*);
    }

private:
    void** baseAddr = nullptr;
    size_t size = 0;
};

class Task {
public:
    static inline AtomicList<Task> schedulingQueue;

    std::atomic<Task*> next = nullptr;
    std::atomic<uint64_t> events = 0;
    uint64_t key; // (~((events + key) ^ events)) & key
    Stack stack;
    ExecutionContext executionContext;
    AtomicList<Task>* list;

    virtual ~Task() { }
    virtual void run() { }
    void yield();
};

void Task::yield() {
    Task* nextTask = schedulingQueue.popFront();
    if (nextTask->executionContext.isValid()) {
        executionContext.yieldTo(&(nextTask->executionContext));
    } else {
        executionContext.bootstrap(
            [](void* data) {
                Task* task = (Task*)data;
                task->run();
                task->yield();
            },
            nextTask->stack.origin(), nextTask);
    }
}
