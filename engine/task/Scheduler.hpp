#pragma once

#include "containers/Allocator32Bit.hpp"
#include "containers/AtomicArrayBuilder.hpp"
#include "containers/FlatSet.hpp"
#include "containers/StableVector.hpp"
#include "task/Heap.hpp"
#include "util/log.hpp"
#include <bitset>
#include <list>

namespace task {

using tickdiff_t = int16_t;
using refcnt_t = int16_t;
using urefcnt_t = uint16_t;

class Scheduler {
public:
    struct Root {
        uint32_t id = 0;
        bool operator==(const Root& other) const { return id == other.id; }
        explicit operator bool() const { return id != 0; }
        bool operator==(std::nullptr_t) const { return !(bool)*this; }
        Root& operator=(std::nullptr_t) {
            id = 0;
            return *this;
        }
    };
    struct Task {
        uint32_t id : 24 = 0;
        uint32_t rootId : 8 = 0;
        Task() { }
        Task(std::nullptr_t) { }
        Task(Root root, uint32_t id)
            : id(id), rootId(root.id) { }
        bool operator==(const Task& other) const { return id == other.id && rootId == other.rootId; }
        explicit operator bool() const { return id != 0 && rootId != 0; }
        bool operator==(std::nullptr_t) const { return !(bool)*this; }
        Task& operator=(std::nullptr_t) {
            id = 0;
            rootId = 0;
            return *this;
        }
        Root root() { return { rootId }; }
    };

    static constexpr size_t MAX_TASKS = 1 << 15;
    static constexpr size_t MAX_TICKS = Heap::MAX_TICKS;
    static constexpr size_t MAX_ROOTS = 64;

    Scheduler(int_t maxRoots);

    Root newRoot(int_t startTick = 0);
    void signalRoot(Root root, ArrayView<int_t> sourceTicks);
    Task newTask(Root root, int_t tick);
    void allowExternals(Root root1, Root root2) {
        allowExternalsOneWay(root1, root2);
        allowExternalsOneWay(root2, root1);
    }
    void allowExternalsOneWay(Root source, Root target);

    void signalSelfInitial(Task task, int_t tick) { signalSelf(task, tick, 0); }
    bool isReady(Task task, int_t tick) { return signal(task, tick).load(std::memory_order_relaxed) == SIGNALED_BY_SELF_FLAG; }
    void updateDependencyCount(Task target, Task source, tickdiff_t sourceTicksDelay, refcnt_t delta);
    void signalDependencies(Task task, int_t tick);

    Heap::TickHelper heap(Root root, utick_t tick) { return rootHeader(root).heap.tick(tick); }
    void*& payload(Task task) { return taskDependencies(task).payload; }

    int_t& externalTick(Root source, Root target, utick_t sourceTick) { return rootHeader(source).externalTicks[heap(source, sourceTick)][target.id]; }

protected:
    virtual void queue(Task task, int_t tick) = 0;

private:
    using TaskOrExternal = Task;

    static constexpr urefcnt_t SIGNALED_BY_SELF_FLAG = (urefcnt_t)1 << (std::numeric_limits<urefcnt_t>::digits - 1);
    static constexpr urefcnt_t SIGNALED_BY_ROOT_FLAG = SIGNALED_BY_SELF_FLAG;

    struct Dependency {
        TaskOrExternal task;
        tickdiff_t delay;
        refcnt_t refCount;

        bool operator==(const Dependency& other) const {
            return task == other.task && delay == other.delay;
        }
    };
    struct DependencyQueueItem {
        TaskOrExternal task;
        tickdiff_t delay;
        refcnt_t refCountDelta;

        Dependency asEmptyDependency() { return { task, delay, 0 }; }
    };
    struct RootQueueItem {
        TaskOrExternal task;
        utick_t firstSourceTick;
        refcnt_t refCountDelta;
    };

    struct TaskSignals {
        std::atomic<urefcnt_t> signal;
        std::atomic<refcnt_t> signalDelta;
    };
    struct TaskDependencies {
        FlatSet<Dependency> internalDep;
        FlatSet<Dependency> externalDep;
        AtomicArrayBuilder<DependencyQueueItem> queue;
        refcnt_t signalCount = 0;
        void* payload;
    };

    struct ExternalTaskInfo {
        Task task;
        refcnt_t signalCount = 0;
    };

    struct ExternalHeader {
        AtomicArrayBuilder<RootQueueItem> queue;
        FlatSet<RootQueueItem> delayedQueue;
        FlatSet<TaskOrExternal> externals;
        StableVector<ExternalTaskInfo, MAX_TASKS> externalTasks;
        AtomicStableVector<std::atomic<urefcnt_t>, MAX_TASKS> signals;
    };

    struct RootHeader {
        int_t tick = 0;
        Heap::Helper heap;
        std::unique_ptr<ExternalHeader[]> externalHeaders;
        Ptr32<int_t[]> externalTicks;
        Ptr32<AtomicArrayBuilder<RootQueueItem>> rootQueue;
        uint32_t nextTask = 1;
        StableVector<TaskDependencies, MAX_TASKS> taskDependencies;
        AtomicStableVector<TaskSignals, MAX_TASKS> taskSignals;
    };

    uint32_t nextRoot = 1;

    int_t maxRoots;
    std::unique_ptr<RootHeader[]> rootHeaders;
    std::unique_ptr<std::unique_ptr<Heap>[]> heaps;

    RootHeader& rootHeader(Root root) { return rootHeaders[root.id]; }

    TaskDependencies& taskDependencies(Task task) { return rootHeader(task.root()).taskDependencies[task.id]; }
    AtomicArrayBuilder<DependencyQueueItem>& dependencyQueue(Task task) { return taskDependencies(task).queue; }
    FlatSet<Dependency>& internalDependencies(Task task) { return taskDependencies(task).internalDep; }
    FlatSet<Dependency>& externalDependencies(Task task) { return taskDependencies(task).externalDep; }
    refcnt_t& signalCount(Task task) { return taskDependencies(task).signalCount; }

    TaskSignals& taskSignals(Task task, utick_t tick) { return *rootHeader(task.root()).taskSignals[task.id][heap(task.root(), tick)]; }
    std::atomic<urefcnt_t>& signal(Task task, utick_t tick) { return taskSignals(task, tick).signal; }
    std::atomic<refcnt_t>& signalDelta(Task task, utick_t tick) { return taskSignals(task, tick).signalDelta; }

    std::atomic<urefcnt_t>& externalSignal(Root source, TaskOrExternal target, utick_t sourceTick) { return *externalHeader(source, target.root()).signals[target.id][heap(source, sourceTick)]; }

    ExternalHeader& externalHeader(Root source, Root target) { return rootHeader(source).externalHeaders[target.id]; }
    AtomicArrayBuilder<RootQueueItem>& rootQueue(Root source, Root target) { return externalHeader(source, target).queue; }
    FlatSet<RootQueueItem>& rootQueueDelayed(Root source, Root target) { return externalHeader(source, target).delayedQueue; }
    FlatSet<TaskOrExternal>& externals(Root source, Root target) { return externalHeader(source, target).externals; }

    Task taskFromExternal(Root source, TaskOrExternal target) { return externalHeader(source, target.root()).externalTasks[target.id].task; }
    refcnt_t& externalSignalCount(Root source, TaskOrExternal target) { return externalHeader(source, target.root()).externalTasks[target.id].signalCount; }

    void signalSelf(Task task, int_t targetTick, refcnt_t signalCount);
    void signalInternal(Task target, int_t targetTick, urefcnt_t delta);
    void signalExternal(Root source, TaskOrExternal target, utick_t sourceTick, urefcnt_t delta);
    void signalExternalFromRoot(Root source, TaskOrExternal target, utick_t sourceTick, refcnt_t signalCount);
};

}
