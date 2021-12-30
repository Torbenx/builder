#include "task/DependencyGraph.hpp"

namespace {

static void verifyValidTick(uint8_t harmonic, utick_t tick) {
    VERIFY(Task, tick & (((tick_t)1 << harmonic) - 1) == 0);
}

/* ==============> Concurrency <============== */

void DependencyGraph::signalTarget(CompleteTarget target, utick_t tick, urefcnt_t delta) {
    verifyValidTick(target.harmonic, tick);
    urefcnt_t i = signal(target, tick).fetch_add(delta, std::memory_order_acq_rel);
    i += delta;
    if (i == TARGET_UNLOCKED_FLAG)
        onDepedenciesFulfilled(target, tick);
}

void DependencyGraph::unlockTarget(TargetNodeImpl& impl, CompleteTarget target, utick_t tick) {
    verifyValidTick(target.harmonic, tick);
    impl.updates.insert(updateQueue(target).take());
    TickExpander expander = tickExpander(target.cluster());
    impl.updates.apply(
        [&](auto update) {
            if (update.isDependencyUpdate()) {
                for (int_t i = update.dependency.relHarmonic; i < NUM_HARMONICS; i++)
                    impl.refCounts[i] += update.dependency.refCount;
            }
        },
        tick, expander);
    utick_t nextTick = tick + ((utick_t)1 << target.harmonic);
    int relHarmonic = std::countr_zero(nextTick) - target.harmonic;
    signalTarget(target, nextTick, TARGET_UNLOCKED_FLAG - impl.refCounts[relHarmonic]);
}

void sDependencyGraph::sendMetadataUpdate(CompleteSource source, Dependency dep, utick_t tick, Metadata metadata) {
    utick_t targetTick = ceilTickToHarmonic(dep.absHarmonic, tick);
    updateQueue(dep.target(source.cluster())).pushBack({ metadata, tick + dep.delay });
}

// must be called every tick
void DependencyGraph::signalSource(SourceNodeImpl& impl, CompleteSource source, utick_t tick, Metadata metadata) {
    verifyValidTick(source.harmonic, tick);
    utick_t nextTick = tick + ((utick_t)1 << source.harmonic);
    impl.updates.insert(updateQueue(source).take(nextTick));
    TickExpander expander = tickExpander(source.cluster());
    impl.updates.apply(
        [&](auto update) {
            auto dep = update.dep;
            int_t idx = impl.dependencies.find(dep);
            if (idx == impl.dependencies.size()) {
                impl.insert(dep);
                if (!metadata.hasChanged()) // lets ensure we only send one update
                    sendMetadataUpdate(source, dep, tick, metadata);
            } else
                impl.dependencies[idx].refCount += dep.refCount;
        });
    impl.dependencies.removeIf([](auto dep) { return dep.refCount == 0; });

    if (metadata.hasChanged()) {
        for (auto dep : impl.dependencies)
            sendMetadataUpdate(source, dep, tick, metadata);
    }

    uint8_t includedHarmonics = std::countr_zero(tick);
    for (auto dep : impl.dependencies) {
        if (dep.absHarmonic <= includedHarmonics) {
            signalTarget(dep.target(source.cluster()), tick + dep.delay, dep.refCount);
        }
    }
}

// must never be called after unlockTarget(..., target, sourceTick + ticksDelay)
void DependencyGraph::updateDependencyWithinCluster(CompleteSource source, CompleteTarget target, utick_t sourceUtick, tickdiff_t ticksDelay, refcnt_t delta) {
    verifyValidTick(source.harmonic, sourceUtick);
    verifyValidTick(target.harmonic, tick);
    int_t firstEmittingTick, firstReceivingTick;
    TickExpander expander = tickExpander(source.cluster());
    int_t sourceTick = expander.tick(sourceUtick);
    int_t relHarmonic = (int_t)target.harmonic - (int_t)source.harmonic;
    updateQueue(source).pushBack({ { target, ticksDelay, delta }, sourceUTick }, [&](utick_t currentSourceUtick) {
        int_t currentSourceTick = expander.tick(currentSourceUtick);
        firstEmittingTick = std::max(currentSourceTick, sourceTick);
        firstReceivingTick = firstEmittingTick + ticksDelay;
        signal(target, firstReceivingTick).fetch_sub(delta, std::memory_order_relaxed);
    });
    updateQueue(target).pushBack({ (uint8_t)std::max<int_t>(0, -relHarmonic), delta, (utick_t)firstReceivingTick });
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

CompleteSource DependencyGraph::updateDependency(CompleteSource source, CompleteTarget target, utick_t sourceTick, tickdiff_t targetTicksDelay, refcnt_t delta) {
    verifyValidTick(source.harmonic, sourceTick);
    verifyValidTick(target.harmonic, targetTicksDelay);
    auto [mappedSource, mappedSourceTick] = maybeMap(source, target.cluster(), sourceTick);
    updateDependencyWithinCluster(mappedSource, target, mappedSourceTick, targetTicksDelay, delta);
    return mappedSource;
}

void DependencyGraph::onDepedenciesFulfilled(CompleteTarget target, utick_t tick) {
    verifyValidTick(target.harmonic, tick);
    if (target.isMapping()) {
        auto& impl = findMappedSource(target, tick);
        // we only get here after unlockTarget in beginClusterTick().
        utick_t sourceTick = translateTick(target.cluster(), impl.source.cluster(), tick); // what about the harmonic?
        signalSource(impl, impl.source, sourceTick);
    } else {
        scheduleTask(target.asTask(), tick);
    }
}

void DependencyGraph::beginClusterTick(Cluster cluster, utick_t tick, const std::bitset<MAX_GROUPS>& firstTimeUsingTick) {
    auto& impl = clusterImpl(cluster);
    for (auto mappedTarget : impl.getMappedTargets()) {
        if (firstTimeUsingTick.test(mappedTarget.group().index())) {
            utick_t mappedTick = translateTick(cluster.harmonic(0), mappedTarget.group(), tick);
            unlockTarget(mappedTarget, mappedTick, 1);
        } else {
            // can't do unlockTarget(mappedTarget, mappedTick, 0)
            utick_t mappedTick = ;
            auto& impl = findMappedSource(mappedTarget, mappedTick);
            utick_t sourceTick = tick; // mapped sources are always in the 0th harmonic
            signalSource(impl, impl.source, sourceTick);
        }
    }
}

std::pair<CompleteSource, utick_t> DependencyGraph::maybeMap(CompleteSource source, Cluster cluster, utick_t sourceTick) {
    if (source.cluster() == cluster)
        return { source, sourceTick };

    // mapped sources are always in the 0th harmonic
    utick_t mappedSourceTick = translateTickCeil(source.cluster(), cluster, sourceTick);

    auto& impl = clusterImpl(cluster);
    auto [sourceMapping, wasAllocated] = impl.findOrCreateMapping(source, cluster, mappedSourceTick);
    if (!wasAllocated)
        return { sourceMapping, mappedSourceTick };

    auto mappedTarget = clusterImpl(source.cluster()).createMappedTarget(source, sourceMapping, sourceTick);
    updateDependencyWithinCluster(source, mappedTarget, sourceTick, 0, 1);
    impl.registerMappedTarget(mappedTarget, mappedSourceTick);
    return { sourceMapping, mappedSourceTick };
}

MappedSourceNodeImpl& DependencyGraph::findMappedSource(CompleteTarget target, utick_t tick) {
    return clusterImpl(target.cluster()).findMappedSource(target, tick);
}

}
