#pragma once

#include "events/ExecutionContext.hpp"
#include "util/AtomicList.hpp"
#include "util/log.hpp"
#include "util/time.hpp"
#include <atomic>
#include <thread>

class Stack {
public:
    Stack() {
        size = 2048;
        baseAddr = new void*[size];
    }

    ~Stack() {
        delete[] baseAddr;
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

class Task : public AtomicListItem<Task> {
public:
    static inline AtomicList<Task> schedulingQueue;

    std::atomic<uint64_t> events = 0;
    uint64_t key; // (~((events + key) ^ events)) & key
    Stack stack;
    ExecutionContext executionContext;
    AtomicList<Task>* list;

    virtual ~Task() { }
    virtual void run() { }
    static void yieldWith(ExecutionContext& executionContext);
    void yield() { yieldWith(executionContext); }
};

class ShutdownTask;

class Scheduler {
private:
    static inline std::unordered_map<std::thread::id, ExecutionContext*> entryPoints; //lock this
    friend class ShutdownTask;

public:
    void enter() {
        std::thread::id myThread = std::this_thread::get_id();
        ExecutionContext executionContext;
        entryPoints[myThread] = &executionContext;
        Task::yieldWith(executionContext);
    }
};

class ShutdownTask : public Task {
public:
    virtual void run() override {
        std::thread::id myThread = std::this_thread::get_id();
        ExecutionContext me;
        me.yieldTo(Scheduler::entryPoints[myThread]);
    }
};

class SchedulerThead : public Scheduler, public std::thread {
public:
    SchedulerThead()
        : std::thread([&]() {
            enter();
        }) { }
};

void Task::yieldWith(ExecutionContext& executionContext) {
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
