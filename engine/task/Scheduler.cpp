#include "task/Scheduler.hpp"

namespace task {

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
        signalInternal(taskFromExternal(source, target), ticksFor(source, sourceTick)[target.root().id], 1);
    }
}

void Scheduler::signalRoots(std::bitset<MAX_ROOTS> execute) {
    /*for (int_t i = 0; i < maxRoots + 1; i++) {
        if (execute[i]) {
            headers[i].tick += 1;
        }
    }
    for (int_t i = 0; i < maxRoots + 1; i++) {
        Root root;
        root.id = i;
        if (!isRootUsed(root))
            continue;

        utick_t tick = headers[i].tick;
        int_t* ticksFor = this->ticksFor(root, tick);
        for (int_t j = 0; j < maxRoots + 1; j++) {
            ticksFor[j] = headers[j].tick;
        }

        FlatSet<syncid_t>& externals = tasksWithExternalDependencies(root);

        auto queue = rootQueue(root, tick).take(1);
        for (auto item : queue) {
            utick_t targetTick = ticksFor[item.task.root().id] + item.delay;
            signal(item.task, targetTick).fetch_add(item.refCountDelta, std::memory_order_relaxed);
            signalDelta(item.task, targetTick).fetch_add(item.refCountDelta, std::memory_order_relaxed);

            if (!externals.has(item.syncId))
                externals.insert(item.syncId);
        }
        AtomicArrayBuilder<RootDependencyQueueItem>::free(queue);

        auto allocator = heap(root, tick).temporaryAllocator();
        for (int_t i = 0; i < externals.size(); i++) {
            Ptr32<RootAndTaskSyncHeader> ptr = rootAndTaskSync(root, externals[i], tick).exchange(SIGNALED_BY_ROOT_SENTINEL, std::memory_order_acq_rel);
            if (ptr) {
                RootAndTaskSyncHeader* header = ptr[allocator];
                if (header->size > 0) {
                    signalExternalDependencies(root, tick, { header->ptr[allocator], header->size });
                } else {
                    externals.removeAt(i);
                    i--;
                }
            }
        }
    }*/
}

void Scheduler::signalDependencies(Task source, int_t tick) {
    utick_t sourceTick = tick;

    auto queue = dependencyQueue(source).take(sourceTick + 1);

    FlatSet<Dependency>& internals = this->internalDependencies(source);
    FlatSet<Dependency>& externals = this->externalDependencies(source);
    for (auto item : queue) {
        auto& depedencies = item.task.root() == source.root() ? internals : externals;
        int_t i = depedencies.getOrInsert(item.asEmptyDependency());
        depedencies[i].refCount += item.refCountDelta;
    }
    AtomicArrayBuilder<DependencyQueueItem>::free(queue);

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
void Scheduler::updateDependencyCount(Task target, Task source, tickdiff_t targetTicksDelay, refcnt_t delta) {
    if (target.root() == source.root()) {
        dependencyQueue(source).pushBack({ target, targetTicksDelay, delta },
            [&](utick_t sourceTick) {
                utick_t targetTick = sourceTick + targetTicksDelay;
                signal(target, targetTick).fetch_sub(delta, std::memory_order_relaxed);
                signalDelta(target, targetTick).fetch_sub(delta, std::memory_order_relaxed);
            });
    } else {
        TaskOrExternal external = target;
        external.id = 0; // find or allocate external id
        dependencyQueue(source).pushBack({ external, targetTicksDelay, delta },
            [&](utick_t sourceTick) {
                utick_t targetTick = rootQueue(source.root(), external.root()).pushBack({ external, targetTicksDelay, delta, sourceTick }) + targetTicksDelay;
                if (false /* allocate */) {
                    signal(target, targetTick).fetch_sub(1, std::memory_order_relaxed);
                    signalDelta(target, targetTick).fetch_sub(1, std::memory_order_relaxed);
                }
            });
    }
}

}