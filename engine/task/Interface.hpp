#pragma once

#include "task/Global.hpp"

namespace task {

namespace detail {
    // make target depend on source
    // ticksAhead = number of ticks target is ahead of source
    inline void updateDependencyCount(TaskHandle target, TaskHandle source, int16_t ticksAhead, int16_t delta) {
        global->signal(tick + ticksAhead, target).fetch_add(delta, std::memory_order_relaxed);
        global->signalDelta(tick + ticksAhead, target).fetch_add(delta, std::memory_order_relaxed);
        uint32_t tick = global->dependencyQueue(source).pushBack({ target, ticksAhead, delta });
        log("updateDependencyCount: delta={}, ticksAhead={}, tick={}", delta, ticksAhead, tick);
    }
    inline void createDependency(TaskHandle target, TaskHandle source, int16_t ticksAhead) { updateDependencyCount(target, source, ticksAhead, 1); }
    inline void deleteDependency(TaskHandle target, TaskHandle source, int16_t ticksAhead) { updateDependencyCount(target, source, ticksAhead, -1); }

    inline void deleteDependencyFromCurrent(TaskHandle target, int16_t ticksAhead) { deleteDependency(target, currentTask, ticksAhead); }

    namespace {
        // the current interface a method is being called on
        thread_local struct {
            TaskHandle task;
            int16_t ticksAhead = 0;

            inline int16_t createDependency(TaskHandle source, int16_t sourceTicksAhead) {
                int16_t ticksAhead = this->ticksAhead - sourceTicksAhead;
                VERIFY(Task, ticksAhead >= 0);
                detail::createDependency(this->task, source, ticksAhead);
                return ticksAhead;
            }
        } currentInterfaceTask;
    }

    template<typename T>
    struct InterfaceWrapper {
        T* ptr;
        T* operator->() { return ptr; }

        InterfaceWrapper(TaskHandle task, int16_t ticksAhead, T* ptr)
            : ptr(ptr) {
            VERIFY(Task, currentInterfaceTask.task == nullptr);
            currentInterfaceTask.task = task;
            currentInterfaceTask.ticksAhead = ticksAhead;
        }
        InterfaceWrapper(const InterfaceWrapper&) = delete;
        InterfaceWrapper(InterfaceWrapper&&) = delete;
        InterfaceWrapper& operator=(const InterfaceWrapper&) const = delete;
        InterfaceWrapper& operator=(InterfaceWrapper&&) const = delete;
        ~InterfaceWrapper() {
            currentInterfaceTask = { nullptr, 0 };
        }
    };
}

template<typename T>
class InterfaceParameter;

template<typename T>
class InterfaceRef {
public:
    InterfaceRef(const InterfaceRef<T>&) = delete;
    InterfaceRef& operator=(const InterfaceRef<T>&) = delete;

    InterfaceRef() { }
    InterfaceRef(InterfaceRef<T>&& other)
        : task(other.task), interface(other.interface), ticksAhead(other.ticksAhead) {
        other.clearFields();
    }
    InterfaceRef& operator=(InterfaceRef<T>&& other) {
        deleteDependency();
        task = other.task;
        interface = other.interface;
        ticksAhead = other.ticksAhead;
        other.clearFields();
    }
    InterfaceRef& operator=(std::nullptr_t) {
        deleteDependency();
        clearFields();
    }

    detail::InterfaceWrapper<T> operator->() { return { task, ticksAhead, interface[detail::global->instanceAllocator(detail::tick + ticksAhead)] }; }

    ~InterfaceRef() { deleteDependency(); }

    static InterfaceRef<T> createFromCurrent(TaskHandle target, Ptr32<T> interface, int16_t ticksAhead) {
        detail::createDependency(target, detail::currentTask, ticksAhead);
        return { target, interface, ticksAhead };
    }

public:
    TaskHandle task;
    Ptr32<T> interface;
    int16_t ticksAhead = 0;

    friend class InterfaceParameter<T>;

    InterfaceRef(TaskHandle task, Ptr32<T> interface, int16_t ticksAhead)
        : task(task), interface(interface), ticksAhead(ticksAhead) { }

    void clearFields() {
        task = nullptr;
        interface = nullptr;
        ticksAhead = 0;
    }

    void deleteDependency() {
        if (task) {
            detail::deleteDependencyFromCurrent(task, ticksAhead);
        }
    }
};

template<typename T>
class InterfaceParameter {
public:
    InterfaceParameter(const InterfaceRef<T>& ref)
        : task(ref.task), interface(ref.interface) {
        ticksAhead = detail::currentInterfaceTask.createDependency(ref.task, ref.ticksAhead);
    }
    InterfaceParameter(InterfaceParameter&& other)
        : task(other.task), interface(other.interface), ticksAhead(other.ticksAhead) {
        other.task = nullptr;
        other.interface = nullptr;
        other.ticksAhead = 0;
    }
    InterfaceParameter(const InterfaceParameter&) = delete;
    InterfaceParameter& operator=(const InterfaceParameter&) = delete;
    InterfaceParameter& operator=(InterfaceParameter&&) = delete;

    ~InterfaceParameter() { VERIFY(Task, task == nullptr); }

private:
    TaskHandle task;
    Ptr32<T> interface;
    int16_t ticksAhead = 0;
};

template<typename T>
struct DataRef {
    TaskHandle task;
    Ptr32<T> data;
    int16_t ticksBehind = 0;

    DataRef(const DataRef<T>&) = delete;
    DataRef& operator=(const DataRef<T>&) = delete;
};

}
