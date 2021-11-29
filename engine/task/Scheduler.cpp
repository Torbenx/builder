#include "task/Scheduler.hpp"

namespace task {

Scheduler::Root Scheduler::newRoot(int_t startTick) {
    Root root { nextRoot++ };
    heaps[root.id].reset(new Heap());
    RootHeader& header = headers[root.id];
    header.heap = heaps[root.id]->helper();
    auto allocator = headers[root.id].heap.tick(startTick).persistentAllocator();
    header.tick = startTick;
    header.ticksFor = make32<int_t[]>(allocator, maxRoots + 1);
    header.rootQueue = make32<AtomicArrayBuilder<RootQueueItem>>(allocator);
    return root;
}

Scheduler::Task Scheduler::newTask(Root root, int_t tick) {
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

void Scheduler::signalInternal(Task target, int_t targetTick, urefcnt_t delta) {
    // acquire the writes of all previous tasks and release our own writes
    urefcnt_t j = signal(target, targetTick).fetch_add(delta, std::memory_order_acq_rel);
    j += delta;
    log("{{{}}} at {} signaled {} time(s), waiting for {} more signals", (uint32_t)target.id, targetTick, delta, j);
    if (j == SIGNALED_BY_SELF_FLAG) {
        queue(target, targetTick);
    }
}

void Scheduler::signalSelf(Task task, int_t tick, refcnt_t signalCount) {
    signalInternal(task, tick, SIGNALED_BY_SELF_FLAG - (urefcnt_t)signalCount);
}

void Scheduler::signalExternal(Root source, TaskOrExternal target, utick_t sourceTick, urefcnt_t delta) {
    urefcnt_t j = externalSignal(source, target, sourceTick).fetch_add(delta, std::memory_order_acq_rel);
    j += delta;
    if (j == SIGNALED_BY_ROOT_FLAG) {
        signalInternal(taskFromExternal(source, target), externalTick(source, target.root(), sourceTick), 1);
    }
}

void Scheduler::signalExternalFromRoot(Root source, TaskOrExternal target, utick_t sourceTick, refcnt_t signalCount) {
    signalExternal(source, target, sourceTick, SIGNALED_BY_ROOT_FLAG - (urefcnt_t)signalCount);
}

void Scheduler::signalRoot(Root root, ArrayView<int_t> sourceTicks) {
    int_t tick = headers[root.id].tick++;
    for (int_t j = 0; j < maxRoots + 1; j++) {
        Root source;
        source.id = j;
        int_t sourceTick = sourceTicks[source.id];
        externalTick(source, root, sourceTick) = tick;

        auto queue = rootQueue(source, root).take(tick + 1);
        auto& delayedQueue = rootQueueDelayed(source, root);
        delayedQueue.insert(queue);
        queue.free();

        for (auto item : delayedQueue) {
            if (item.firstSourceTick <= sourceTick) {
                externalSignalCount(source, item.task) += item.refCountDelta;
            }
        }

        for (auto item : externals(source, root)) {
            signalExternalFromRoot(source, item, sourceTick, externalSignalCount(source, item));
        }
    }
}

void Scheduler::signalDependencies(Task source, int_t tick) {
    utick_t sourceTick = tick;

    auto queue = dependencyQueue(source).take(sourceTick + 1);

    FlatSet<Dependency>& internals = internalDependencies(source);
    FlatSet<Dependency>& externals = externalDependencies(source);
    for (auto item : queue) {
        auto& depedencies = item.task.root() == source.root() ? internals : externals;
        int_t i = depedencies.getOrInsert(item.asEmptyDependency());
        depedencies[i].refCount += item.refCountDelta;
    }
    queue.free();

    internals.removeIf([](Dependency d) { return d.refCount == 0; });
    for (auto dep : internals) {
        signalInternal(dep.task, tick + dep.delay, dep.refCount);
    }

    externals.removeIf([](Dependency d) { return d.refCount == 0; });
    for (auto dep : externals) {
        signalExternal(source.root(), dep.task, sourceTick + dep.delay, dep.refCount);
    }

    refcnt_t& signalCount = this->signalCount(source);
    signalCount += signalDelta(source, sourceTick).load(std::memory_order_relaxed);
    signalSelf(source, tick + 1, signalCount);
}

// target.root() must depend on the calling task
void Scheduler::updateDependencyCount(Task target, Task source, tickdiff_t sourceTicksDelay, refcnt_t delta) {
    if (target.root() == source.root()) {
        dependencyQueue(source).pushBack({ target, sourceTicksDelay, delta },
            [&](utick_t sourceTick) {
                utick_t targetTick = sourceTick + sourceTicksDelay;
                signal(target, targetTick).fetch_sub(delta, std::memory_order_relaxed);
                signalDelta(target, targetTick).fetch_sub(delta, std::memory_order_relaxed);
            });
    } else {
        TaskOrExternal external = target;
        external.id = 0; // find or allocate external id
        dependencyQueue(source).pushBack({ external, sourceTicksDelay, delta },

            [&](utick_t sourceTick) {
                rootQueue(source.root(), external.root()).pushBack({ external, sourceTick + sourceTicksDelay, delta },

                    [&](utick_t targetTick) {
                        if (false /* allocate */) {
                            signal(target, targetTick).fetch_sub(1, std::memory_order_relaxed);
                            signalDelta(target, targetTick).fetch_sub(1, std::memory_order_relaxed);
                        }
                    });
            });
    }
}

}