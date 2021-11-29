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

    Scheduler(int_t maxRoots)
        : maxRoots(maxRoots) {
        headers.reset(new RootHeader[maxRoots + 1]);
        heaps.reset(new std::unique_ptr<Heap>[maxRoots + 1]);
    }

    Root newRoot(int_t startTick = 0);
    void signalRoot(Root root, ArrayView<int_t> sourceTicks);
    Task newTask(Root root, int_t tick);

    void signalSelfInitial(Task task, int_t tick) { return signalSelf(task, tick, signalCount(task)); }
    bool isReady(Task task, int_t tick) { return signal(task, tick).load(std::memory_order_relaxed) == 1; }
    void updateDependencyCount(Task target, Task source, tickdiff_t sourceTicksDelay, refcnt_t delta);
    void signalDependencies(Task task, int_t tick);

    Heap::TickHelper heap(Root root, utick_t tick) { return headers[root.id].heap.tick(tick); }
    void*& payload(Task task) { return dependencyBucket(task).payload[task.id & TASK_BUCKET_MASK]; }

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

    static constexpr size_t TASK_BUCKET_SHIFT = 8;
    static constexpr size_t TASK_BUCKET_SIZE = (size_t)1 << TASK_BUCKET_SHIFT;
    static constexpr size_t TASK_BUCKET_COUNT = MAX_TASKS >> TASK_BUCKET_SHIFT;
    static constexpr size_t TASK_BUCKET_MASK = TASK_BUCKET_SIZE - 1;
    struct TaskSignalBucket {
        std::atomic<urefcnt_t> signals[TASK_BUCKET_SIZE];
        std::atomic<refcnt_t> signalDeltas[TASK_BUCKET_SIZE];
    };
    struct TaskDependencyBucket {
        FlatSet<Dependency> internalDeps[TASK_BUCKET_SIZE];
        FlatSet<Dependency> externalDeps[TASK_BUCKET_SIZE];
        AtomicArrayBuilder<DependencyQueueItem> queues[TASK_BUCKET_SIZE];
        refcnt_t signalCounts[TASK_BUCKET_SIZE] = {};
        void* payload[TASK_BUCKET_SIZE];
    };

    struct RootHeader {
        int_t tick;
        Heap::Helper heap;
        Ptr32<int_t[]> ticksFor;
        Ptr32<AtomicArrayBuilder<RootQueueItem>> rootQueue;
        uint32_t nextTask = 1;
        Ptr32<TaskSignalBucket> signalBuckets[TASK_BUCKET_COUNT];
        TaskDependencyBucket* dependencyBuckets[TASK_BUCKET_COUNT] = {};
    };

    uint32_t nextRoot = 1;

    int_t maxRoots;
    std::unique_ptr<RootHeader[]> headers;
    std::unique_ptr<std::unique_ptr<Heap>[]> heaps;

    TaskDependencyBucket& dependencyBucket(Task task) { return *headers[task.root().id].dependencyBuckets[task.id >> TASK_BUCKET_SHIFT]; }
    AtomicArrayBuilder<DependencyQueueItem>& dependencyQueue(Task task) { return dependencyBucket(task).queues[task.id & TASK_BUCKET_MASK]; }
    FlatSet<Dependency>& internalDependencies(Task task) { return dependencyBucket(task).internalDeps[task.id & TASK_BUCKET_MASK]; }
    FlatSet<Dependency>& externalDependencies(Task task) { return dependencyBucket(task).externalDeps[task.id & TASK_BUCKET_MASK]; }
    refcnt_t& signalCount(Task task) { return dependencyBucket(task).signalCounts[task.id & TASK_BUCKET_MASK]; }

    int_t& externalTick(Root source, Root target, utick_t sourceTick) { return headers[source.id].ticksFor[heap(source, sourceTick)][target.id]; }

    TaskSignalBucket& signalBucket(Task task, utick_t tick) { return *headers[task.root().id].signalBuckets[task.id >> TASK_BUCKET_SHIFT][heap(task.root(), tick)]; }
    std::atomic<urefcnt_t>& signal(Task task, utick_t tick) { return signalBucket(task, tick).signals[task.id & TASK_BUCKET_MASK]; }
    std::atomic<refcnt_t>& signalDelta(Task task, utick_t tick) { return signalBucket(task, tick).signalDeltas[task.id & TASK_BUCKET_MASK]; }

    std::atomic<urefcnt_t>& externalSignal(Root source, TaskOrExternal target, utick_t sourceTick);

    AtomicArrayBuilder<RootQueueItem>& rootQueue(Root source, Root target);
    FlatSet<RootQueueItem>& rootQueueDelayed(Root source, Root traget);
    FlatSet<TaskOrExternal>& externals(Root source, Root target);

    Task taskFromExternal(Root source, TaskOrExternal target);
    refcnt_t& externalSignalCount(Root source, TaskOrExternal target);

    void signalSelf(Task task, int_t targetTick, refcnt_t signalCount);
    void signalInternal(Task target, int_t targetTick, urefcnt_t delta);
    void signalExternal(Root source, TaskOrExternal target, utick_t sourceTick, urefcnt_t delta);
    void signalExternalFromRoot(Root source, TaskOrExternal target, utick_t sourceTick, refcnt_t signalCount);

    bool isRootUsed(Root root) { return (bool)headers[root.id].ticksFor; }
};

}
