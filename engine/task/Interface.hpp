#pragma once

#include "task/Scheduler.hpp"

namespace task {

using TaskHandle = Scheduler::Task;
using RootHandle = Scheduler::Root;

class CoroutineScheduler : public Scheduler {
public:
    CoroutineScheduler()
        : Scheduler(1) { }

    std::pair<TaskHandle, int_t> next() {
        for (int_t i = 0; i < (int_t)MAX_TICKS; i++) {
            if (queues[i].size() > 0) {
                TaskHandle task = queues[i].front();
                queues[i].pop_front();
                return { task, i };
            }
        }
        return { nullptr, 0 };
    }

protected:
    void queue(Task task, int_t tick) {
        queues[tick].push_back(task);
    }

    std::list<TaskHandle> queues[MAX_TICKS];
};

namespace detail {
    inline thread_local CoroutineScheduler* scheduler = nullptr;
    inline thread_local TaskHandle currentTask;
    inline thread_local int_t tick = 0;

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
            activeTask = { task, ticksAhead };
        }
        Wrapper(const Wrapper&) = delete;
        Wrapper(Wrapper&&) = delete;
        Wrapper& operator=(const Wrapper&) = delete;
        Wrapper& operator=(Wrapper&&) = delete;
        ~Wrapper() {
            activeTask = { nullptr, 0 };
        }
    };
}

template<typename T>
struct RefDescriptor {
    TaskHandle task;
    Ptr32<T> ptr;
    int16_t ticksAhead = 0;

    RefDescriptor(TaskHandle task, Ptr32<T> ptr, int16_t ticksAhead)
        : task(task), ptr(ptr), ticksAhead(ticksAhead) { }

    explicit operator bool() { return task != nullptr; }
};

template<typename T>
class Ref : public RefDescriptor<T> {
public:
    Ref(const Ref<T>&) = delete;
    Ref& operator=(const Ref<T>&) = delete;

    Ref() { }
    Ref(Ref<T>&& other)
        : Ref((RefDescriptor<T>)other) {
        other.clearFields();
    }
    Ref& operator=(Ref<T>&& other) {
        deleteDependency();
        this->task = other.task;
        this->ptr = other.ptr;
        this->ticksAhead = other.ticksAhead;
        other.clearFields();
        return *this;
    }
    Ref& operator=(std::nullptr_t) {
        deleteDependency();
        clearFields();
        return *this;
    }

    detail::Wrapper<T> operator->() { return { this->task, this->ticksAhead, this->get() }; }

    ~Ref() { deleteDependency(); }

    static Ref<T> createFromCurrent(TaskHandle target, Ptr32<T> ptr, int16_t ticksAhead) {
        detail::createDependency(target, detail::currentTask, ticksAhead);
        return { RefDescriptor<T> { target, ptr, ticksAhead } };
    }

    static Ref<T> createToCurrent(TaskHandle source, Ptr32<T> ptr, int16_t ticksAhead) {
        detail::createDependency(detail::currentTask, source, ticksAhead);
        return { RefDescriptor<T> { source, ptr, ticksAhead } };
    }

protected:
    Ref(const RefDescriptor<T>& ref)
        : RefDescriptor<T>(ref) { }

    T* get() {
        int_t tick;
        if (task.root() == detail::currentTask.root()) {
            tick = detail::tick;
        } else {
            tick = 0; // fix me
        }
        return this->ptr[detail::scheduler->heap(this->task.root(), detail::tick + this->ticksAhead)];
    }

    void deleteDependency() {
        if (this->task) {
            detail::deleteDependencyFromCurrent(this->task, this->ticksAhead);
        }
    }

    void clearFields() {
        this->task = nullptr;
        this->ptr = nullptr;
        this->ticksAhead = 0;
    }
};

template<typename T>
class SelfRef {
public:
    Ptr32<T> ptr;

    SelfRef(const SelfRef&) = delete;
    SelfRef& operator=(const SelfRef&) = delete;
    SelfRef(Ptr32<T> ptr)
        : ptr(ptr) { }
    SelfRef() { }

    T* operator->() { return ptr[heap()]; }
    RefDescriptor<T> asRef(int16_t ticksAhead) { return { detail::currentTask, ptr, ticksAhead }; }

    ~SelfRef() { /* how to free this? */ }

private:
    Heap::TickHelper heap() { return detail::scheduler->heap(detail::currentTask.root(), detail::tick); }
};

template<typename T, bool currentIsSource>
class Parameter : public Ref<T> {
public:
    Parameter(const RefDescriptor<T>& ref)
        : Ref<T>(RefDescriptor<T> { ref.task, ref.ptr, detail::activeTask.createDependency(ref.task, ref.ticksAhead, currentIsSource) }) { }
    Parameter(Parameter&& other)
        : Ref<T>(std::move(other)) { }

    Parameter(const Parameter&) = delete;
    Parameter& operator=(const Parameter&) = delete;
    Parameter& operator=(Parameter&&) = delete;

    ~Parameter() { VERIFY(Task, this->task == nullptr); }
};
template<typename T>
using InterfaceParameter = Parameter<T, true>;
template<typename T>
using DataParameter = Parameter<T, false>;

}
