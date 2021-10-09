#pragma once

#include "util/log.hpp"

namespace Tag {
struct Task {
    static constexpr char tag[] = "task";
    static constexpr bool verifyEnabled = true;
};
}

namespace task {

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

class Scheduler {
public:
};

}
