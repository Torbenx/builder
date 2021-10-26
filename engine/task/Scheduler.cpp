#include "task/Scheduler.hpp"

namespace task {

void Scheduler::signalInternal(Task target, int_t targetTick, refcnt_t delta) {
    // acquire the writes of all previous tasks and release our own writes
    refcnt_t j = signal(target, targetTick).fetch_add(delta, std::memory_order_acq_rel);
    j += delta;
    log("{{{}}} at {} signaled {} time(s), waiting for {} more signals", (uint32_t)target.id, targetTick, -delta, j - 1);
    if (j == 1) {
        queue(target, targetTick);
    }
}

void Scheduler::signalExternalDependencies(Root source, utick_t sourceTick, ArrayView<Dependency> externals) {
    int_t* ticks = ticksFor(source, sourceTick);
    for (Dependency dep : externals) {
        signalInternal(dep.task, ticks[dep.task.root().id] + dep.delay, -dep.refCount);
    }
}

void Scheduler::signalRoots(std::bitset<MAX_ROOTS> execute) {
    for (int_t i = 0; i < maxRoots + 1; i++) {
        if (execute[i]) {
            headers[i].tick += 1;
        }
    }
    for (int_t i = 0; i < maxRoots + 1; i++) {
        Root root(i);
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
    }
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

    bool hadExternals = externals.size() > 0;
    internals.removeIf([](Dependency d) { return d.refCount == 0; });
    externals.removeIf([](Dependency d) { return d.refCount == 0; });

    for (int_t i = 0; i < internals.size(); i++) {
        auto dep = internals[i];
        signalInternal(dep.task, tick + dep.delay, -dep.refCount);
    }

    syncid_t syncId = this->syncId(source).load(std::memory_order_relaxed);
    if (hadExternals) {
        auto& flag = rootAndTaskSync(source.root(), syncId, sourceTick);
        if (flag.load(std::memory_order_acquire) == SIGNALED_BY_ROOT_SENTINEL) {
            signalExternalDependencies(source.root(), sourceTick, externals);
        } else {
            auto allocator = heap(source.root(), sourceTick).temporaryAllocator();
            auto header = make32<RootAndTaskSyncHeader>(allocator);
            auto data = make32<Dependency[]>(allocator, externals.size());
            header[allocator]->size = externals.size();
            header[allocator]->ptr = data;
            std::memcpy(data[allocator], externals.data(), externals.size() * sizeof(Dependency));
            if (flag.exchange(nullptr, std::memory_order_acq_rel)) {
                allocator.free32(data.offset);
                allocator.free32(header.offset);
                signalExternalDependencies(source.root(), sourceTick, externals);
            }
        }
    }

    refcnt_t& signalCount = this->signalCount(source);
    signalCount += signalDelta(source, sourceTick).load(std::memory_order_relaxed);
    signalInternal(source, tick + 1, signalCount + 1);
}

// target must depend on source.root()
void Scheduler::updateDependencyCount(Task target, Task source, tickdiff_t targetTicksDelay, refcnt_t delta) {
    if (target.root() == source.root()) {
        dependencyQueue(source).pushBack({ target, targetTicksDelay, delta },
            [&](utick_t sourceTick) {
                utick_t targetTick = sourceTick + targetTicksDelay;
                signal(target, targetTick).fetch_add(delta, std::memory_order_relaxed);
                signalDelta(target, targetTick).fetch_add(delta, std::memory_order_relaxed);
            });
    } else {
        dependencyQueue(source).pushBack({ target, targetTicksDelay, delta },
            [&](utick_t sourceTick) {
                syncid_t syncId = this->syncId(source).load(std::memory_order_relaxed);
                if (syncId == 0) {
                    syncid_t newSyncId = syncIdAllocator.allocateId();
                    headers[source.root().id].rootAndTaskSyncContainer.ensureExists(newSyncId);
                    this->syncId(source).compare_exchange_strong(syncId, newSyncId, std::memory_order_relaxed);
                    if (syncId == 0) {
                        syncId = newSyncId;
                    } else {
                        syncIdAllocator.freeId(newSyncId);
                    }
                }
                uint32_t takenByRoot = rootQueue(source.root(), sourceTick).pushBackIf({ target, targetTicksDelay, delta, syncId }, [](uint32_t takenByRoot) {
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

}