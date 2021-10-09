#include "task/Promise.hpp"

namespace task {

void PromiseBase::signalDependencies() {
    auto queue = global->dependencyQueue(task).take((uint32_t)(detail::tick + 1));

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
            // acquire the writes of all previous tasks and release our own writes
            auto j = global->signal(dep.task, detail::tick + dep.ticksAhead).fetch_sub(dep.refCount, std::memory_order_acq_rel);
            j -= dep.refCount;
            //log("signaling {} for tick={}, waiting for {} more dependencies", dep.task.id, targetTick, j);
            if (j == 0) {
                global->queue(dep.task, dep.ticksAhead);
            }
        }
    }

    AtomicArrayBuilder<Global::DependencyQueueItem>::free(queue);

    signalCount += global->signalDelta(task, detail::tick).load(std::memory_order_relaxed);
    auto mySignalCount = signalCount;

    auto j = global->signal(task, detail::tick + 1).fetch_add(mySignalCount, std::memory_order_acq_rel);
    j += mySignalCount;
    if (j == 0) {
        global->queue(task, 1);
    }
    //log("waiting for {} more dependencies", j);
}

}