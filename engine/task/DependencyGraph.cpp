#include "task/DependencyGraph.hpp"

namespace {

/* ==============> Concurrency <============== */

void DependencyGraph::signalTarget(Group group, Target target, utick_t tick, urefcnt_t delta) {
    urefcnt_t i = signal(group, target, tick).fetch_add(delta, std::memory_order_acq_rel);
    i += delta;
    if (i == TARGET_UNLOCKED_FLAG)
        onDepedenciesFulfilled(group, target, tick);
}

void DependencyGraph::unlockTarget(Group group, Target target, utick_t tick, refcnt_t signalCount) {
    signalTarget(group, target, tick, TARGET_UNLOCKED_FLAG - signalCount);
}

void DependencyGraph::updateDependency(Group group, Source source, Target target, utick_t tick, tickdiff_t delay, refcnt_t delta) {
    utick_t firstEmittingTick, firstReceivingTick;
    updateQueue(group, source).pushBack({ source, target, delay, delta, tick }, [&](utick_t currentSourceTick) {
        firstEmittingTick = std::max(currentSourceTick, tick);
        firstReceivingTick = firstEmittingTick + delay;
        impl()->signal(target, firstReceivingTick).fetch_sub(delta, std::memory_order_relaxed);
    });
    updateQueue(group, target).pushBack({ group, target, delta, firstReceivingTick });
    return { firstEmittingTick, firstReceivingTick };
}

/* ==============> ? <============== */

/*
 * Tasks providing data to external tasks don't depend on anything.
 * Tasks receiving data from external tasks depend on their root,
   since they must know from which sourceTick to take it.
 * Tasks holding an interface to an external task depend on their root,
   since they must know where to write messages.
 * Tasks providing an interface to an external task depend on their root,
   since they must know which sourceTick to depend on.
 */

void DependencyGraph::onTaskTickFinish(Promise& prom, Group group, Task task, utick_t tick) {
    auto expander = tickExpander(group);
    prom.sourceUpdates.insert(updateQueue(group, Source { task }).task(tick + 1));
    prom.sourceUpdates.apply(
        [&](auto update) {
            auto& dep = prom.depedencies.getOrInsert(update.asEmptyDependency());
            dep.refCount += update.delta;
        },
        tick, expander);
    prom.targetUpdates.insert(updateQueue(group, Target { task }).take());
    prom.targetUpdates.apply(
        [&](auto update) {
            prom.refCount += update.delta;
        },
        tick, expander);

    for (auto dep : prom.dependencies)
        signalTarget(group, dep.target, tick + dep.delay, dep.refCount);
    unlockTarget(group, Target { task }, tick + 1, prom.refCount);
}

void DependencyGraph::onDepedenciesFulfilled(Group group, Target target, utick_t tick) {
    if (target.isMapping()) {
        // We only get here after beginTick(..., extGroup, extTick),
        // i.e. we depend on target's root.
        auto [trnGroup, trnTarget] = translateMapping(group, target);
        utick_t trnTick = translateTick(group, trnGroup, tick);
        signalTarget(trnGroup, trnTarget, trnTick, EXTERNAL_REFERENCE_REF_COUNT);
    } else {
        scheduleTask(group, target.asTask(), tick);
    }
}

// We always have an iterface on target.
Target DependencyGraph::updateDependency(Group sourceG, Task source, Group targetG, Task target, utick_t sourceTick, tickdiff_t sourceTicksDelay, refcnt_t delta) {
    if (sourceG == targetG) {
        updateDependency(sourceG, Source { source }, Target { target }, sourceTick, sourceTicksDelay, delta);
        return Target { target };
    }
    // Since we have an interface to target, this depends on source's root.
    // So this will always happend after beginTick(..., sourceG, sourceTick).
    auto [mapping, wasAllocated] = findOrCreateMapping(sourceG, targetG, target, sourceTick);
    if (wasAllocated) {
        // When source is ahead the target may have 0 dependencies for a while,
        // but target must still start depending on its root now.
        utick_t targetTick = translateTick(sourceG, targetG, sourceTick);
        updateDependency(targetG, Source::ROOT, Target { target }, targetTick, 0, EXTERNAL_REFERENCE_REF_COUNT);
    }
    updateDependency(sourceG, Source { source }, mapping, sourceTick, sourceTicksDelay, delta);
    return mapping;
}

void DependencyGraph::beginGroupTick(GroupHeader& header, Group group, utick_t tick, const std::bitset<MAX_GROUPS>& firstTimeUsingTick) {
    auto expander = tickExpander(group);
    header.sourceUpdates.insert(mappingsCreationQueue(group).take(tick + 1));
    header.sourceUpdates.apply(
        [&](auto update) {
            header.depedencies.insert(update.asEmptyMappedDependency());
        },
        tick, expander);
    header.targetUpdates.insert(mappingsUpdateQueue(group).take());
    header.targetUpdates.apply(
        [&](auto update) {
            Target target = translateMapping(update.sourceGroup, update.target);
            auto& dep = header.depedencies.get(target);
            auto& info = dep.referencingGroups.getOrInsert({ update.sourceGroup, update.target });
            info.refCount += update.delta;
        },
        tick, expander);

    // handle removing stuff...

    for (auto& dep : header.dependencies) {
        refcnt_t directSignals = 0;
        for (auto info : dep.referencingGroups) {
            // We only depend on a group the first time the tick is used, then
            // the dependency is carried implicitly by the fact that each task
            // depends on its previous iterations.
            if (firstTimeUsingTick.test(info.group.index()))
                unlockTarget(info.group, info.mapping, translateTick(group, info.group, tick), info.refCount);
            else
                directSignals += EXTERNAL_REFERENCE_REF_COUNT;
        }
        if (directSignals != 0)
            signalTarget(group, dep.target, tick, directSignals);
    }
}

}
