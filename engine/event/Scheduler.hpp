#pragma once

#include "util/Callable.hpp"
#include "Script.hpp"
#include "util/types.hpp"
#include <atomic>

namespace evt {

constexpr int_t MAX_CONCURRENT_TICKS = 7;

struct Scheduler;
struct SchedulerImpl;
using SchedulingItem = stx::BasicCallback<Scheduler>;

struct Scheduler {
    SchedulerImpl* scheduler = nullptr;
    Scheduler() = default;
    Scheduler(SchedulerImpl* scheduler)
        : scheduler(scheduler) { }
    void schedule(SchedulingItem, int_t tick = evt::tick());
    void tickComplete(int_t tick = evt::tick());
    explicit operator bool() { return (bool)scheduler; }
};

void startScheduler(SchedulingItem, int_t startTick);
void enterScheduler();

template<typename T, int_t N>
struct RingBuffer {
    T storage[N] = {};
    int_t index(int_t i) const { return (size_t)i % N; }
    constexpr T& operator[](int_t i) { return storage[index(i)]; }
    constexpr const T& operator[](int_t i) const { return storage[index(i)]; }
};
template<typename T>
using TickRingBuffer = RingBuffer<T, MAX_CONCURRENT_TICKS + 1>;

}

namespace evt::dep {

using cnt_t = int16_t;
constexpr cnt_t READY_COUNT = std::numeric_limits<cnt_t>::max();
constexpr cnt_t COUNT_BOUND = ((int_t)READY_COUNT + 1) / 2;
constexpr cnt_t INVALID_SIGNAL_INCREMENT = COUNT_BOUND / 2;

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
    template<stx::Callable<Target, void, Scheduler> Callback>
    Target(const Callback& callback) {
        setCallback(callback);
    }
    template<stx::Callable<Target, void, Scheduler> Callback>
    void setCallback(const Callback&) {
        schedulingItem.setCallback(this, [](Target* thiz, Scheduler scheduler) {
            stx::callWithSource<void, Scheduler>(Callback {}, *thiz, scheduler);
            thiz->completeTick(scheduler);
        });
    }

    bool unlockForResult(int_t tick = evt::tick()) {
        return applySignalForResult(READY_COUNT - dependencyCount, tick);
    }
    void unlock(Scheduler scheduler, int_t tick = evt::tick()) {
        VERIFY(Event, (bool)scheduler);
        if (unlockForResult(tick))
            scheduler.schedule(schedulingItem, tick);
    }

    void completeTick(Scheduler scheduler);

    void incrementDependencyCount(cnt_t count = 1, int_t tick = evt::tick()) {
        VERIFY(Event, count >= 0);
        auto& thisTick = perTick[tick];
        thisTick.countDiff.fetch_add(count, std::memory_order_relaxed);
        thisTick.counter.fetch_sub(count, std::memory_order_seq_cst);
    }
    void decrementDependencyCount(Scheduler scheduler, cnt_t count = 1, int_t tick = evt::tick()) {
        perTick[tick].countDiff.fetch_sub(count, std::memory_order_relaxed);
        applySignal(scheduler, count, tick);
    }

    bool applySignalForResult(cnt_t count = 1, int_t tick = evt::tick()) {
        cnt_t i = perTick[tick].counter.fetch_add(count, std::memory_order_seq_cst);
        i += count;
        return i == READY_COUNT;
    }
    void applySignal(Scheduler scheduler, cnt_t count = 1, int_t tick = evt::tick()) {
        if (applySignalForResult(count, tick)) {
            if (scheduler)
                scheduler.schedule(schedulingItem, tick);
            else
                perTick[tick].counter.fetch_add(INVALID_SIGNAL_INCREMENT, std::memory_order_seq_cst);
        }
    }
    bool blockedFromInvalidSignal(int_t tick) const {
        return perTick[tick].counter.load(std::memory_order_relaxed) == (cnt_t)(READY_COUNT + INVALID_SIGNAL_INCREMENT);
    }
    ~Target() {
        VERIFY(Event, blockedFromInvalidSignal(evt::tick()));
    }
};

struct Signal {
    Target* target = nullptr;
    int_t tickShift = 0;
    bool operator==(const Signal&) const = default;
};
struct Source;
struct Dependency {
    Source* source = nullptr;
    Signal signal = {};

    Dependency() = default;
    Dependency(Source* source, Signal signal)
        : source(source), signal(signal) { }
    Dependency(const Dependency&) = delete;
    Dependency(Dependency&& other)
        : source(other.source), signal(other.signal) {
        other.source = nullptr;
        other.signal = {};
    }
    ~Dependency() { destroy({}); }
    Dependency& operator=(Dependency&& other) {
        destroy({});
        source = other.source;
        signal = other.signal;
        other.source = nullptr;
        other.signal = {};
        return *this;
    }
    void destroy(Scheduler);
    void leak() {
        source = nullptr;
        signal = {};
    }
};

struct Source {
    std::vector<Signal> targets;
    Source() = default;
    Source(const Source&) = delete;
    ~Source() {
        VERIFY(Event, targets.empty());
    }

    [[nodiscard]] Dependency createDependency(Target& target, int_t tickShift);
    [[nodiscard]] Dependency createDependency(Target& target) {
        return createDependency(target, 0);
    }
    [[nodiscard]] Dependency createNextTickDependency(Target& target) {
        return createDependency(target, 1);
    }
    void removeDependency(Scheduler scheduler, Target& target, int_t tickShift);
    void removeDependency(Scheduler scheduler, Target& target) {
        removeDependency(scheduler, target, 0);
    }
    void removeNextTickDependency(Scheduler scheduler, Target& target) {
        removeDependency(scheduler, target, 1);
    }

    void signalTargets(Scheduler scheduler) {
        for (auto target : targets)
            target.target->applySignal(scheduler, 1, evt::tick() + target.tickShift);
    }
};

template<typename Callback>
struct SimpleTarget : Target {
    SimpleTarget() {
        this->setCallback<stx::WithDerived<SimpleTarget, Callback>>({});
    }

    void tailSignal(Scheduler scheduler) {
        if (applySignalForResult()) {
            stx::callWithSource<void, Scheduler>(Callback {}, *this, scheduler);
            completeTick(scheduler);
        }
    }
};

}

namespace evt {

struct Task : dep::Source, dep::Target {
    template<stx::Callable<Task, void, Scheduler> Callback>
    Task(const Callback& callback) {
        setCallback(callback);
    }
    template<stx::Callable<Task, void, Scheduler> Callback>
    void setCallback(const Callback&) {
        Target::setCallback(stx::withDerived<Task>([](Task& thiz, Scheduler scheduler) {
            stx::callWithSource<void, Scheduler>(Callback {}, thiz, scheduler);
            thiz.signalTargets(scheduler);
        }));
    }
};

}