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
    using tickdiff_t = int16_t;
    using refcnt_t = int16_t;
    using utick_t = Heap::utick_t;

    static constexpr size_t MAX_TASKS = 1 << 15;
    static constexpr size_t MAX_TICKS = Heap::MAX_TICKS;
    static constexpr size_t MAX_ROOTS = 64;

    Scheduler(int_t maxRoots)
        : maxRoots(maxRoots) {
        headers.reset(new RootHeader[maxRoots + 1]);
        heaps.reset(new std::unique_ptr<Heap>[maxRoots + 1]);
    }

    Root newRoot(int_t startTick = 0) {
        Root root { nextRoot++ };
        heaps[root.id].reset(new Heap());
        RootHeader& header = headers[root.id];
        header.heap = heaps[root.id]->helper();
        auto allocator = headers[root.id].heap.tick(startTick).persistentAllocator();
        header.tick = startTick;
        header.ticksFor = make32<int_t[]>(allocator, maxRoots + 1);
        header.rootQueue = make32<AtomicArrayBuilder<RootDependencyQueueItem>>(allocator);
        return root;
    }
    void signalRoots(std::bitset<MAX_ROOTS> execute);

    Task newTask(Root root, int_t tick) {
        RootHeader& header = headers[root.id];
        Task task { root, header.nextTask++ };
        int_t bucket = task.id >> TASK_BUCKET_SHIFT;
        int_t bucketOffset = task.id & TASK_BUCKET_MASK;
        if (!header.signalBuckets[bucket]) {
            header.signalBuckets[bucket] = make32<TaskSignalBucket>(header.heap.tick(tick).persistentAllocator());
            header.dependencyBuckets[bucket] = new TaskDependencyBucket;
        }
        return task;
    }

    // returns wether the task should be queued for tick
    bool signalSelfInitial(Task task, int_t tick) { return signalInternalRet(task, tick, 1); }
    void updateDependencyCount(Task target, Task source, tickdiff_t targetTicksDelay, refcnt_t delta);
    void signalDependencies(Task task, int_t tick);

    Heap::TickHelper heap(Root root, utick_t tick) { return headers[root.id].heap.tick(tick); }

protected:
    virtual void queue(Task task, int_t tick) = 0;

private:
    using syncid_t = uint32_t;

    struct Dependency {
        Task task;
        tickdiff_t delay;
        refcnt_t refCount;

        bool operator==(const Dependency& other) const {
            return task == other.task && delay == other.delay;
        }
    };
    struct DependencyQueueItem {
        Task task;
        tickdiff_t delay;
        refcnt_t refCountDelta;

        Dependency asEmptyDependency() { return { task, delay, 0 }; }
    };
    struct RootDependencyQueueItem : DependencyQueueItem {
        syncid_t syncId;
    };
    struct RootAndTaskSyncHeader {
        uint32_t size;
        Ptr32<Dependency[]> ptr;
    };
    static constexpr Ptr32<RootAndTaskSyncHeader> SIGNALED_BY_ROOT_SENTINEL = { std::numeric_limits<uint32_t>::max() };

    static constexpr size_t TASK_BUCKET_SHIFT = 8;
    static constexpr size_t TASK_BUCKET_SIZE = (size_t)1 << TASK_BUCKET_SHIFT;
    static constexpr size_t TASK_BUCKET_COUNT = MAX_TASKS >> TASK_BUCKET_SHIFT;
    static constexpr size_t TASK_BUCKET_MASK = TASK_BUCKET_SIZE - 1;
    struct TaskSignalBucket {
        std::atomic<refcnt_t> signals[TASK_BUCKET_SIZE];
        std::atomic<refcnt_t> signalDeltas[TASK_BUCKET_SIZE];
    };
    struct TaskDependencyBucket {
        FlatSet<Dependency> internalDeps[TASK_BUCKET_SIZE];
        FlatSet<Dependency> externalDeps[TASK_BUCKET_SIZE];
        AtomicArrayBuilder<DependencyQueueItem> queues[TASK_BUCKET_SIZE];
        refcnt_t signalCounts[TASK_BUCKET_SIZE] = {};
        std::atomic<syncid_t> externalSyncIds[TASK_BUCKET_SIZE] = {};
    };

    using RootAndTaskSyncContainter = AtomicStableVector<std::atomic<Ptr32<RootAndTaskSyncHeader>>, MAX_TASKS, Heap::EmptyInplacePersistentAllocator>;
    struct RootHeader {
        int_t tick;
        Heap::Helper heap;
        FlatSet<syncid_t> externalDependencies;
        Ptr32<int_t[]> ticksFor;
        Ptr32<AtomicArrayBuilder<RootDependencyQueueItem>> rootQueue;
        uint32_t nextTask = 1;
        Ptr32<TaskSignalBucket> signalBuckets[TASK_BUCKET_COUNT];
        TaskDependencyBucket* dependencyBuckets[TASK_BUCKET_COUNT] = {};
        Ptr32<RootAndTaskSyncContainter> rootAndTaskSyncContainer;
    };

    uint32_t nextRoot = 1;

    int_t maxRoots;
    std::unique_ptr<RootHeader[]> headers;
    std::unique_ptr<std::unique_ptr<Heap>[]> heaps;
    IdAllocator<syncid_t> syncIdAllocator;

    TaskDependencyBucket& dependencyBucket(Task task) { return *headers[task.root().id].dependencyBuckets[task.id >> TASK_BUCKET_SHIFT]; }
    AtomicArrayBuilder<DependencyQueueItem>& dependencyQueue(Task task) { return dependencyBucket(task).queues[task.id & TASK_BUCKET_MASK]; }
    FlatSet<Dependency>& internalDependencies(Task task) { return dependencyBucket(task).internalDeps[task.id & TASK_BUCKET_MASK]; }
    FlatSet<Dependency>& externalDependencies(Task task) { return dependencyBucket(task).externalDeps[task.id & TASK_BUCKET_MASK]; }
    refcnt_t& signalCount(Task task) { return dependencyBucket(task).signalCounts[task.id & TASK_BUCKET_MASK]; }
    std::atomic<syncid_t>& syncId(Task task) { return dependencyBucket(task).externalSyncIds[task.id & TASK_BUCKET_MASK]; }

    FlatSet<syncid_t>& tasksWithExternalDependencies(Root root) { return headers[root.id].externalDependencies; }

    int_t* ticksFor(Root root, utick_t tick) { return headers[root.id].ticksFor[heap(root, tick)]; }
    AtomicArrayBuilder<RootDependencyQueueItem>& rootQueue(Root root, utick_t tick) { return *headers[root.id].rootQueue[heap(root, tick)]; }

    TaskSignalBucket& signalBucket(Task task, utick_t tick) { return *headers[task.root().id].signalBuckets[task.id >> TASK_BUCKET_SHIFT][heap(task.root(), tick)]; }
    std::atomic<refcnt_t>& signal(Task task, utick_t tick) {
        //log("signal[{{{}}}, {}] = {}", (uint32_t)task.id, tick, signalBucket(task, tick).signals[task.id & TASK_BUCKET_MASK].load());
        return signalBucket(task, tick).signals[task.id & TASK_BUCKET_MASK];
    }
    std::atomic<refcnt_t>& signalDelta(Task task, utick_t tick) { return signalBucket(task, tick).signalDeltas[task.id & TASK_BUCKET_MASK]; }

    std::atomic<Ptr32<RootAndTaskSyncHeader>>& rootAndTaskSync(Root root, syncid_t syncId, utick_t tick) { return *headers[root.id].rootAndTaskSyncContainer[heap(root, tick)]->operator[](syncId)[heap(root, tick)]; }

    bool signalInternalRet(Task target, int_t targetTick, refcnt_t delta);
    void signalInternal(Task target, int_t targetTick, refcnt_t delta) {
        if (signalInternalRet(target, targetTick, delta)) {
            queue(target, targetTick);
        }
    }
    void signalExternalDependencies(Root source, utick_t sourceTick, ArrayView<Dependency> externals);

    bool isRootUsed(Root root) { return (bool)headers[root.id].ticksFor; }
};

}
