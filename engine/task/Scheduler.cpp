#include "task/Scheduler.hpp"

namespace task {

Scheduler::Scheduler(int_t maxRoots)
    : maxRoots(maxRoots) {
    rootHeaders.reset(new RootHeader[maxRoots + 1]);
    heaps.reset(new std::unique_ptr<Heap>[maxRoots + 1]);
}

Scheduler::Root Scheduler::newRoot(int_t startTick) {
    Root root { nextRoot++ };
    heaps[root.id].reset(new Heap());
    RootHeader& header = rootHeader(root);
    header.heap = heaps[root.id]->helper();
    auto allocator = header.heap.tick(startTick).persistentAllocator();
    header.tick = startTick;
    header.externalTicks = make32<int_t[]>(allocator, maxRoots + 1);
    header.rootQueue = make32<AtomicArrayBuilder<RootQueueItem>>(allocator);
    header.externalHeaders.reset(new ExternalHeader[maxRoots] {});
    return root;
}

Scheduler::Task Scheduler::newTask(Root root, int_t tick) {
    RootHeader& header = rootHeader(root);
    Task task { root, header.nextTask++ };
    header.taskDependencies.ensureExists(task.id); // not safe?
    header.taskSignals.ensureExists(task.id, heap(root, tick).persistentAllocator());
    return task;
}

void Scheduler::allowExternalsOneWay(Root source, Root target) {
    ExternalHeader& header = externalHeader(source, target);
    // nothing to?
}

void Scheduler::signalInternal(Task target, int_t targetTick, urefcnt_t delta) {
    // acquire the writes of all previous tasks and release our own writes
    urefcnt_t j = signal(target, targetTick).fetch_add(delta, std::memory_order_acq_rel);
    j += delta;
    if (j >= SIGNALED_BY_SELF_FLAG / 2) {
        log("{{{}}} at {} signaled {} time(s), waiting for {} more signal(s)", (uint32_t)target.id, targetTick, delta, SIGNALED_BY_SELF_FLAG - j);
    } else {
        log("{{{}}} at {} signaled {} time(s), {} time(s) in total", (uint32_t)target.id, targetTick, delta, j);
    }
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
    int_t tick = rootHeader(root).tick++;
    for (int_t j = 0; j < maxRoots + 1; j++) {
        Root source;
        source.id = j;
        int_t sourceTick = sourceTicks[source.id];
        externalTick(source, root, sourceTick) = tick;

        auto queue = rootQueue(source, root).take(tick + 1);
        auto& delayedQueue = rootQueueDelayed(source, root);
        delayedQueue.insert(*queue);
        queue.release();

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

void Scheduler::TaskInternals::signalDependencies(Scheduler& scheduler, Task source, int_t tick) {
    utick_t sourceTick = tick;

    auto queue = scheduler.dependencyQueue(source).take(sourceTick + 1);

    for (auto item : *queue) {
        auto& depedencies = item.task.root() == source.root() ? internals : externals;
        int_t i = depedencies.getOrInsert(item.asEmptyDependency());
        depedencies[i].refCount += item.refCountDelta;
    }
    queue.release();

    internals.removeIf([](Dependency d) { return d.refCount == 0; });
    for (auto dep : internals) {
        scheduler.signalInternal(dep.task, tick + dep.delay, dep.refCount);
    }

    externals.removeIf([](Dependency d) { return d.refCount == 0; });
    for (auto dep : externals) {
        scheduler.signalExternal(source.root(), dep.task, sourceTick + dep.delay, dep.refCount);
    }

    signalCount += scheduler.signalDelta(source, sourceTick).load(std::memory_order_relaxed);
    scheduler.signalSelf(source, tick + 1, signalCount);
}

void Scheduler::updateDependencyCount(Task target, Task source, tickdiff_t sourceTicksDelay, refcnt_t delta) {
    if (target.root() == source.root()) {
        dependencyQueue(source).pushBack({ target, sourceTicksDelay, delta },
            [&](utick_t sourceTick) {
                utick_t targetTick = sourceTick + sourceTicksDelay;
                signal(target, targetTick).fetch_sub(delta, std::memory_order_relaxed);
                signalDelta(target, targetTick).fetch_add(delta, std::memory_order_relaxed);
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
                            signalDelta(target, targetTick).fetch_add(1, std::memory_order_relaxed);
                        }
                    });
            });
    }
}

}