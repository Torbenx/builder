#pragma once

#include "containers/MemoryAllocator32.hpp"
#include "util/log.hpp"
#include "util/coroutines.hpp"
#include "util/uninitialized.hpp"
#include <type_traits>

#ifdef interface
// apparently mingw's rcp.h defines this
#undef interface
#endif

namespace Tag {
struct Task {
    static constexpr char tag[] = "task";
    static constexpr bool verifyEnabled = true;
};
}

namespace task_detail {
inline uninitialized<MemoryAllocator32> g_allocator;
inline MemoryAllocator32& allocator() { return g_allocator.value; }
}

template<typename Interface = void>
class TaskPromise;
template<typename Interface = void>
class TaskPromiseBase;

template<typename Interface = void>
class Task {
public:
    using promise_type = TaskPromise<Interface>;

    ~Task() {
        VERIFY(Task, ptr == nullptr);
    }

private:
    TaskPromise<Interface>* ptr;

    TaskPromise<Interface>* take() {
        TaskPromise<Interface>* r = ptr;
        ptr = nullptr;
        return r;
    }

    template<typename Inf>
    friend class TaskPromise;
    template<typename Inf>
    friend class TaskPromiseBase;
};

template<typename Interface = void>
class TaskRef {
public:

    TaskRef() { }
    TaskRef(uint32_t offset)
        : offset(offset) { }

    operator TaskRef<void>() const { return { offset }; }
    TaskPromise<Interface>* operator->() { return (TaskPromise<Interface>*)task_detail::allocator().address(offset); }
    TaskPromise<Interface>& operator*() { return *(TaskPromise<Interface>*)task_detail::allocator().address(offset); }
    stdx::coroutine_handle<TaskPromise<Interface>> coroutine() { return stdx::coroutine_handle<TaskPromise<Interface>>::from_promise(&(*this)); }
    explicit operator bool() const { return offset != 0; }
    bool operator==(const TaskRef& other) const { return offset == other.offset; }

    uint32_t offset = 0;
};

template<typename Interface = void>
class dependent_task {
public:
    std::add_lvalue_reference_t<Interface> interface() requires(!std::is_same_v<Interface, void>) {
        return target->interface;
    }
    dependent_task() { }
    dependent_task(TaskRef<> base, TaskRef<Interface> target)
        : base(base), target(target) { }
    dependent_task(const dependent_task&) = delete;
    dependent_task(dependent_task&& other) {
        free();
        base = other.base;
        target = other.target;
        other.base = nullptr;
        other.target = nullptr;
    }
    ~dependent_task() { free(); }

    operator dependent_task<void>() { return { base, target }; }

private:
    template<typename Inf>
    friend class TaskPromise;
    template<typename Inf>
    friend class TaskPromiseBase;

    TaskRef<> base = {};
    TaskRef<Interface> target = {};

    void free() { }
};

template<typename Interface = void>
struct future_task : dependent_task<Interface> {
    using dependent_task<Interface>::dependent_task;
};
template<typename Interface = void>
struct past_task : dependent_task<Interface> {
    using dependent_task<Interface>::dependent_task;
};
