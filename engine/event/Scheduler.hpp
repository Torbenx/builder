#pragma once

#include "Callable.hpp"
#include "Script.hpp"
#include "util/types.hpp"
#include <atomic>

namespace evt {

constexpr int_t MAX_CONCURRENT_TICKS = 8;

struct SchedulerImpl;
struct SchedulingItem;

struct Scheduler {
    SchedulerImpl* scheduler = nullptr;
    Scheduler() = default;
    Scheduler(SchedulerImpl* scheduler)
        : scheduler(scheduler) { }
    void schedule(SchedulingItem, int_t tick = evt::tick());
};

struct SchedulingItem {
    void (*callback)(void*, Scheduler) = nullptr;
    void* data = nullptr;

    SchedulingItem() = default;
    template<typename Data, BasicCallable<void, Data*, Scheduler> Callback>
    SchedulingItem(Data* data, const Callback& callback) {
        setCallback(data, callback);
    }
    explicit operator bool() const { return (bool)callback; }
    void execute(Scheduler scheduler) const;
    template<typename Data, BasicCallable<void, Data*, Scheduler> Callback>
    void setCallback(Data* data, const Callback&) {
        this->data = data;
        this->callback = [](void* data, Scheduler scheduler) {
            Callback {}((Data*)data, scheduler);
        };
    }
};

void runScheduler(SchedulingItem, int_t startTick);

template<typename T, int_t N>
struct RingBuffer {
    T storage[N] = {};
    int_t index(int_t i) const { return (size_t)i % N; }
    constexpr T& operator[](int_t i) { return storage[index(i)]; }
    constexpr const T& operator[](int_t i) const { return storage[index(i)]; }
};
template<typename T>
using TickRingBuffer = RingBuffer<T, MAX_CONCURRENT_TICKS>;

}

namespace evt::dep {

using cnt_t = int16_t;
constexpr cnt_t READY_COUNT = std::numeric_limits<cnt_t>::max();
constexpr cnt_t COUNT_BOUND = ((int_t)READY_COUNT + 1) / 2;

struct Target {
    struct PerTick {
        /* interpretation:
         * -bound   <= counter < bound   --> locked, signaled counter times
         *  bound   <= counter < 2*bound --> unlocked, waiting for 2*bound-counter-1 more signals
         * -2*bound <= counter < -bound  --> impossible
         */
        std::atomic<cnt_t> counter = 0;
        std::atomic<cnt_t> countDiff = 0;
    };
    TickRingBuffer<PerTick> perTick;
    cnt_t dependencyCount = 0;
    SchedulingItem schedulingItem;

    Target() = default;
    Target(const Target&) = delete;
    template<Callable<Target, void, Scheduler> Callback>
    Target(const Callback& callback) {
        setCallback(callback);
    }
    template<Callable<Target, void, Scheduler> Callback>
    void setCallback(const Callback&) {
        schedulingItem.setCallback(this, [](Target* thiz, Scheduler scheduler) {
            callWithSource<void, Scheduler>(Callback {}, thiz, scheduler);
            thiz->completeTick(scheduler);
        });
    }

    void unlock(Scheduler scheduler, int_t tick = evt::tick()) {
        applySignal(scheduler, READY_COUNT - dependencyCount, tick);
    }

    void completeTick(Scheduler scheduler) {
        PerTick& thisTick = perTick[evt::tick()];
        VERIFY(Event, thisTick.counter.load(std::memory_order_relaxed) == READY_COUNT);
        dependencyCount += thisTick.countDiff.load(std::memory_order_relaxed);
        thisTick.counter.store(0, std::memory_order_relaxed);
        thisTick.countDiff.store(0, std::memory_order_relaxed);

        unlock(scheduler, evt::tick() + 1);
    }

    void incrementDependencyCount(cnt_t count = 1) {
        VERIFY(Event, count >= 0);
        auto& tick = perTick[evt::tick()];
        tick.countDiff.fetch_add(count, std::memory_order_relaxed);
        tick.counter.fetch_sub(count, std::memory_order_seq_cst);
    }
    void decrementDependencyCount(Scheduler scheduler, cnt_t count = 1) {
        perTick[evt::tick()].countDiff.fetch_sub(count, std::memory_order_relaxed);
        applySignal(scheduler, count, evt::tick());
    }

    bool applySignalForResult(cnt_t count = 1, int_t tick = evt::tick()) {
        cnt_t i = perTick[tick].counter.fetch_add(count, std::memory_order_seq_cst);
        i += count;
        return i == READY_COUNT;
    }
    void applySignal(Scheduler scheduler, cnt_t count = 1, int_t tick = evt::tick()) {
        if (applySignalForResult(count, tick))
            scheduler.schedule(schedulingItem, tick);
    }
};

struct Source {
    std::vector<Target*> targets;
    Source() = default;
    Source(const Source&) = delete;

    void createDependency(Target& target) {
        targets.push_back(&target);
        target.incrementDependencyCount();
    }
    void removeDependency(Scheduler scheduler, Target& target) {
        auto it = std::find(targets.begin(), targets.end(), &target);
        VERIFY(Event, it != targets.end());
        targets.erase(it);
        target.decrementDependencyCount(scheduler);
    }

    void signalTargets(Scheduler scheduler) {
        for (Target* target : targets)
            target->applySignal(scheduler);
    }
};

template<typename Callback>
struct SimpleTarget : Target {
    SimpleTarget() {
        this->setCallback<WithDerived<SimpleTarget, Callback>>({});
    }

    void tailSignal(Scheduler scheduler) {
        if (applySignalForResult()) {
            callWithSource<void, Scheduler>(Callback {}, this, scheduler);
            completeTick(scheduler);
        }
    }
};

}

namespace evt {

struct Task : dep::Source, dep::Target {
    template<Callable<Task, void, Scheduler> Callback>
    Task(const Callback& callback) {
        setCallback(callback);
    }
    template<Callable<Task, void, Scheduler> Callback>
    void setCallback(const Callback&) {
        Target::setCallback(withDerived<Task>([](Task* thiz, Scheduler scheduler) {
            callWithSource<void, Scheduler>(Callback {}, thiz, scheduler);
            thiz->signalTargets(scheduler);
        }));
    }
};

dep::Target& tickComplete();

}