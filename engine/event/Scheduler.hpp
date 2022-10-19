#pragma once

#include "Event.hpp"
#include "containers/Owned.hpp"
#include "util/Callable.hpp"
#include "util/types.hpp"
#include <atomic>

namespace evt {

struct Scheduler;
struct SchedulerImpl;
using SchedulingItem = stx::BasicCallback<Scheduler>;

struct Scheduler {
    SchedulerImpl* scheduler = nullptr;
    Scheduler() = default;
    Scheduler(SchedulerImpl* scheduler)
        : scheduler(scheduler) { }
    void schedule(SchedulingItem, int_t tick = evt::tick());
    explicit operator bool() { return (bool)scheduler; }
};

void startScheduler(SchedulingItem, int_t startTick);
void enterScheduler();

void postCallback(Source<CallbackEvent>& postTo, stx::BasicCallback<>);
Source<CallbackEvent>& mainCallbackHandler();
ExecutorImpl* mainExecutor();

}