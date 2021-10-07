#include "task/Promise.hpp"

namespace task {

void PromiseBase::signalDependencies() {
    auto queue = global->dependencyQueue(task()).take((uint32_t)(tick + 1));

    for (auto item : queue) {
        int_t i = dependencies.getOrInsert(item.asEmptyDependency());
        dependencies[i].refCount += item.refCountDelta;
    }

    for (int_t i = 0; i < dependencies.size(); i++) {
        auto dep = dependencies[i];
        if (dep.refCount == 0) {
            dependencies.removeAt(i);
            i--;
        } else {
            int_t targetTick = tick + dep.ticksAhead;
            // acquire the writes of all previous tasks and release our own writes
            auto j = global->signal(targetTick, dep.task).fetch_sub(dep.refCount, std::memory_order_acq_rel);
            j -= dep.refCount;
            if (j == 0) {
                global->queue(global->coroutine(dep.task), targetTick);
            }
        }
    }

    tick++;
    AtomicArrayBuilder<Global::DependencyQueueItem>::free(queue);

    signalCount += global->signalDelta(tick, task()).load(std::memory_order_relaxed);
    auto j = global->signal(tick, task()).fetch_add(signalCount, std::memory_order_acq_rel);
    j += signalCount;
    if (j == 0) {
        global->queue(global->coroutine(task()), tick);
    }
}

}