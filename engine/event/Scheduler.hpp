#pragma once

#include "Script.hpp"
#include "util/types.hpp"

namespace evt {

constexpr int_t MAX_CONCURRENT_TICKS = 8;

struct Scheduler;
struct SchedulerHandle;

struct Task {
    void (*function)(SchedulerHandle, void*) = nullptr;
    void* data = nullptr;
    explicit operator bool() const { return (bool)function; }
    void execute(Scheduler* scheduler) const;
};

struct SchedulerHandle {
    Scheduler* scheduler = nullptr;
    SchedulerHandle() = default;
    SchedulerHandle(Scheduler* scheduler)
        : scheduler(scheduler) { }
    void schedule(Task, int_t tick = evt::tick());
};

void runScheduler(Task);

}

namespace scd {
    
}