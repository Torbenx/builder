#pragma once

#include "task/Description.hpp"
#include <list>

namespace task {

template<Description desc>
class Promise;

template<Description desc = Description {}>
class Task {
public:
    using promise_type = Promise<desc>;

    ~Task() {
        VERIFY(Task, ptr == nullptr);
    }

    Task(Promise<desc>* ptr)
        : ptr(ptr) { }

    Promise<desc>* ptr;

    Promise<desc>* take() {
        Promise<desc>* r = ptr;
        ptr = nullptr;
        return r;
    }

    template<Description d>
    friend class Promise;
};

struct TaskHandle {
    uint32_t id = 0;

    bool operator==(const TaskHandle& other) const { return id == other.id; }
    bool operator==(std::nullptr_t) const { return id == 0; }

    TaskHandle& operator=(std::nullptr_t) {
        id = 0;
        return *this;
    }
    TaskHandle(std::nullptr_t) { }
    TaskHandle() { }
    TaskHandle(uint32_t id)
        : id(id) { }

    explicit operator bool() { return id != 0; }
};

}
