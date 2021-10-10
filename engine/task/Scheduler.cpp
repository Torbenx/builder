#include "task/Scheduler.hpp"

namespace task {

void Scheduler::signalInternal(Task target, int_t targetTick, refcnt_t delta) {
    // acquire the writes of all previous tasks and release our own writes
    refcnt_t j = signal(target, targetTick).fetch_add(delta, std::memory_order_acq_rel);
    j -= delta;
    if (j == 0) {
        this->queue(target, targetTick);
    }
}

void Scheduler::signalExternalDependencies(Root source, utick_t sourceTick, ArrayView<Dependency> externals) {
    int_t* ticks = ticksFor(source, sourceTick);
    for (Dependency dep : externals) {
        signalInternal(dep.task, ticks[dep.task.root().id] + dep.delay, -dep.refCount);
    }
}

void Scheduler::signalRoot(Root root, bool execute) {
    if (!execute) {
        return;
    }

    utick_t tick = ++ticks[root.id];

    int_t* ticksFor = this->ticksFor(root, tick);
    for (int_t i = 0; i < maxRoots; i++) {
        ticksFor[i] = ticks[i];
    }
    FlatSet<Task>& externals = tasksWithExternalDependencies(root);

    auto queue = rootQueue(root, tick).take(1);
    for (auto item : queue) {
        utick_t targetTick = ticksFor[item.task.root().id] + item.delay;
        signal(item.task, targetTick).fetch_add(item.refCountDelta, std::memory_order_relaxed);
        signalDelta(item.task, targetTick).fetch_add(item.refCountDelta, std::memory_order_relaxed);

        if (!externals.has(item.source))
            externals.insert(item.source);
    }
    AtomicArrayBuilder<RootDependencyQueueItem>::free(queue);

    for (int_t i = 0; i < externals.size(); i++) {
        Ptr32<RootAndTaskSyncHeader> ptr = rootAndTaskSync(externals[i], tick).exchange(SIGNALED_BY_ROOT_SENTINEL, std::memory_order_acq_rel);
        if (ptr) {
            RootAndTaskSyncHeader* header = ptr[];
            if (header->size > 0) {
                signalExternalDependencies(root, tick, { header->ptr[], header->size });
            } else {
                externals.removeAt(i);
                i--;
            }
        }
    }
}

void Scheduler::signalDependencies(Task source, int_t tick) {
    utick_t sourceTick = tick;

    auto queue = dependencyQueue(source).take(sourceTick + 1);

    FlatSet<Dependency>& internals = this->internalDependencies(source);
    FlatSet<Dependency>& externals = this->externalDependencies(source);
    for (auto item : queue) {
        FlatSet<Dependency>& dependencies = item.task.root() == source.root() ? internals : externals;
        int_t i = dependencies.getOrInsert(item.asEmptyDependency());
        dependencies[i].refCount += item.refCountDelta;
    }

    AtomicArrayBuilder<DependencyQueueItem>::free(queue);

    bool hadExternals = externals.size() > 0;
    internals.removeIf([](Dependency d) { return d.refCount == 0; });
    externals.removeIf([](Dependency d) { return d.refCount == 0; });

    for (int_t i = 0; i < internals.size(); i++) {
        auto dep = internals[i];
        signalInternal(dep.task, dep.delay, -dep.refCount);
    }

    if (hadExternals) {
        auto& flag = rootAndTaskSync(source, sourceTick);
        if (flag.load(std::memory_order_acquire) == SIGNALED_BY_ROOT_SENTINEL) {
            signalExternalDependencies(source.root(), sourceTick, externals);
        } else {
            // create temporary copy of external dependencies
            if (flag.exchange(nullptr, std::memory_order_acq_rel)) {
                // deallocate
                signalExternalDependencies(source.root(), sourceTick, externals);
            }
        }
    }

    refcnt_t& signalCount = this->signalCount(source);
    signalCount += signalDelta(source, sourceTick).load(std::memory_order_relaxed);
    signalInternal(source, 1, signalCount);
}

// target must depend on source.root()
void Scheduler::updateDependencyCount(Task target, Task source, tickdiff_t targetTicksDelay, refcnt_t delta) {
    dependencyQueue(source).pushBack({ target, targetTicksDelay, delta },
        [&](utick_t sourceTick) {
            if (target.root() == source.root()) {
                utick_t targetTick = sourceTick + targetTicksDelay;
                signal(target, targetTick).fetch_add(delta, std::memory_order_relaxed);
                signalDelta(target, targetTick).fetch_add(delta, std::memory_order_relaxed);
                return true;
            }
            uint32_t takenByRoot = rootQueue(source.root(), sourceTick).pushBack({ target, targetTicksDelay, delta, source }, [](uint32_t takenByRoot) {
                return takenByRoot == 0;
            });
            if (takenByRoot) {
                utick_t targetTick = ticksFor(source.root(), sourceTick)[target.root().id] + targetTicksDelay;
                signal(target, targetTick).fetch_add(delta, std::memory_order_relaxed);
                signalDelta(target, targetTick).fetch_add(delta, std::memory_order_relaxed);
            }
            return true;
        });
}

}