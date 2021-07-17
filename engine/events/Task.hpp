#pragma once

#include "containers/AtomicList.hpp"
#include "events/ExecutionContext.hpp"
#include "util/log.hpp"
#include "util/time.hpp"
#include <atomic>
#include <mutex>
#include <thread>

namespace Tag {
struct Events {
    static constexpr char tag[] = "events";
    static constexpr bool verifyEnabled = true;
};
}

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
    virtual void MIGHT_YIELD run() { }
    static void MIGHT_YIELD yieldWith(ExecutionContext& executionContext);
    void MIGHT_YIELD yield() { yieldWith(executionContext); }
};

class ShutdownTask;

class Scheduler {
private:
    static inline std::unordered_map<std::thread::id, ExecutionContext*> entryPoints;
    static inline std::mutex entryPointsMutex;
    friend class ShutdownTask;

public:
    void MIGHT_YIELD enter() {
        std::thread::id myThread = std::this_thread::get_id();
        ExecutionContext executionContext;
        {
            std::lock_guard<std::mutex> g(Scheduler::entryPointsMutex);
            entryPoints[myThread] = &executionContext;
        }
        Task::yieldWith(executionContext);
    }
};

class ShutdownTask : public Task {
public:
    virtual void MIGHT_YIELD run() override {
        std::thread::id myThread = std::this_thread::get_id();
        ExecutionContext* entryPoint;
        {
            std::lock_guard<std::mutex> g(Scheduler::entryPointsMutex);
            auto it = Scheduler::entryPoints.find(myThread);
            VERIFY(Events, it != Scheduler::entryPoints.end());
            entryPoint = it->second;
            Scheduler::entryPoints.erase(it);
        }
        ExecutionContext me;
        me.yieldTo(entryPoint);
    }
};

class SchedulerThead : public Scheduler, public std::thread {
public:
    SchedulerThead()
        : std::thread([&]() {
            enter();
        }) { }
};

void MIGHT_YIELD Task_yieldWith_bootstrap(void* data) {
    Task* task = (Task*)data;
    task->run();
    task->yield();
}

void Task::yieldWith(ExecutionContext& executionContext) {
    Task* nextTask = schedulingQueue.popFront();
    if (nextTask->executionContext.isValid()) {
        executionContext.yieldTo(&(nextTask->executionContext));
    } else {
        executionContext.bootstrap(Task_yieldWith_bootstrap, nextTask->stack.origin(), nextTask);
    }
}
