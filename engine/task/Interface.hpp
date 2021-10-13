#pragma once

#include "task/Scheduler.hpp"

namespace task {

using TaskHandle = Scheduler::Task;
class CoroutineScheduler;

namespace detail {
    inline thread_local CoroutineScheduler* scheduler = nullptr;
    inline thread_local TaskHandle currentTask;
    inline thread_local int_t tick = 0;
}

class CoroutineScheduler final : public Scheduler {
public:
    CoroutineScheduler()
        : Scheduler(1) { }

    void enter() {
        detail::scheduler = this;
    loop:
        for (int_t i = 0; i < (int_t)MAX_TICKS; i++) {
            if (queues[i].size() > 0) {
                TaskHandle task = queues[i].front();
                queues[i].pop_front();
                detail::tick = i;
                detail::currentTask = task;
                coroutines[task.id].resume();
                signalDependencies(task, i);
                goto loop;
            }
        }
    }

    Heap::TickHelper heap(int_t tick = detail::tick) { return Scheduler::heap(root, tick); }

    TaskHandle newTask(stdx::coroutine_handle<> coro) {
        TaskHandle task = Scheduler::newTask(root, detail::tick);
        if (coroutines.size() <= task.id) {
            coroutines.resize(task.id + 1);
        }
        coroutines[task.id] = coro;
        return task;
    }

private:
    std::list<TaskHandle> queues[MAX_TICKS];
    std::vector<stdx::coroutine_handle<>> coroutines;
    Root root = { newRoot() };
    void queue(TaskHandle task, int_t tick) override {
        queues[tick].push_back(task);
    }
    using Scheduler::newTask;
};

namespace detail {

    inline void createDependency(TaskHandle target, TaskHandle source, int16_t ticksAhead) { scheduler->updateDependencyCount(target, source, ticksAhead, 1); }
    inline void deleteDependency(TaskHandle target, TaskHandle source, int16_t ticksAhead) { scheduler->updateDependencyCount(target, source, ticksAhead, -1); }

    inline void deleteDependencyFromCurrent(TaskHandle target, int16_t ticksAhead) { deleteDependency(target, currentTask, ticksAhead); }

    namespace {
        // the current interface a method is being called on
        thread_local struct {
            TaskHandle task;
            int16_t ticksAhead = 0;

            inline int16_t createDependency(TaskHandle task, int16_t ticksAhead, bool currentIsSource) {
                TaskHandle target = currentIsSource ? task : this->task;
                TaskHandle source = currentIsSource ? this->task : task;
                ticksAhead = ticksAhead - this->ticksAhead;
                detail::createDependency(target, source, std::abs(ticksAhead));
                return ticksAhead;
            }
        } activeTask;
    }

    template<typename T>
    struct Wrapper {
        T* ptr;
        T* operator->() { return ptr; }

        Wrapper(TaskHandle task, int16_t ticksAhead, T* ptr)
            : ptr(ptr) {
            VERIFY(Task, activeTask.task == nullptr);
            activeTask.task = task;
            activeTask.ticksAhead = ticksAhead;
        }
        Wrapper(const Wrapper&) = delete;
        Wrapper(Wrapper&&) = delete;
        Wrapper& operator=(const Wrapper&) = delete;
        Wrapper& operator=(Wrapper&&) = delete;
        ~Wrapper() {
            activeTask.task = nullptr;
            activeTask.ticksAhead = 0;
        }
    };
}

template<typename T>
class Ref {
public:
    Ref(const Ref<T>&) = delete;
    Ref& operator=(const Ref<T>&) = delete;

    Ref() { }
    Ref(Ref<T>&& other)
        : task(other.task), ptr(other.ptr), ticksAhead(other.ticksAhead) {
        other.clearFields();
    }
    Ref& operator=(Ref<T>&& other) {
        deleteDependency();
        task = other.task;
        ptr = other.ptr;
        ticksAhead = other.ticksAhead;
        other.clearFields();
    }
    Ref& operator=(std::nullptr_t) {
        deleteDependency();
        clearFields();
    }
    explicit operator bool() { return task != nullptr; }

    T* get() {
        return ptr[detail::scheduler->heap(detail::tick + ticksAhead)];
    }
    detail::Wrapper<T> operator->() { return { this->task, this->ticksAhead, this->get() }; }

    ~Ref() { deleteDependency(); }

    static Ref<T> createFromCurrent(TaskHandle target, Ptr32<T> ptr, int16_t ticksAhead) {
        detail::createDependency(target, detail::currentTask, ticksAhead);
        return { target, ptr, ticksAhead };
    }

public:
    TaskHandle task;
    Ptr32<T> ptr;
    int16_t ticksAhead = 0;

    Ref(TaskHandle task, Ptr32<T> ptr, int16_t ticksAhead)
        : task(task), ptr(ptr), ticksAhead(ticksAhead) { }

    void clearFields() {
        task = nullptr;
        ptr = nullptr;
        ticksAhead = 0;
    }

    void deleteDependency() {
        if (task) {
            detail::deleteDependencyFromCurrent(task, ticksAhead);
        }
    }
};

template<typename T>
class Parameter {
public:
    Parameter(const Ref<T>& ref, bool currentIsSource)
        : task(ref.task), ptr(ref.ptr) {
        ticksAhead = detail::activeTask.createDependency(ref.task, ref.ticksAhead, currentIsSource);
    }
    Parameter(Parameter&& other)
        : task(other.task), ptr(other.ptr), ticksAhead(other.ticksAhead) {
        other.task = nullptr;
        other.ptr = nullptr;
        other.ticksAhead = 0;
    }
    Parameter(const Parameter&) = delete;
    Parameter& operator=(const Parameter&) = delete;
    Parameter& operator=(Parameter&&) = delete;

    ~Parameter() { VERIFY(Task, task == nullptr); }

private:
    TaskHandle task;
    Ptr32<T> ptr;
    int16_t ticksAhead = 0;
};

template<typename T>
struct InterfaceParameter : Parameter<T> {
    using Parameter<T>::Parameter;
    using Parameter<T>::operator=;

    InterfaceParameter(const Ref<T>& ref)
        : Parameter<T>(ref, true) { }
}; 

template<typename T>
struct DataParameter : Parameter<T> {
    using Parameter<T>::Parameter;
    using Parameter<T>::operator=;

    DataParameter(const Ref<T>& ref)
        : Parameter<T>(ref, false) { }
};

}
