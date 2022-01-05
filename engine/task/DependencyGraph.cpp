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
            for (int_t i = update.relHarmonic; i < NUM_HARMONICS; i++)
                impl.refCounts[i] += update.refCount;
        },
        tick, expander);
    utick_t nextTick = tick + ((utick_t)1 << target.harmonic);
    int relHarmonic = std::countr_zero(nextTick) - target.harmonic;
    signalTarget(target, nextTick, TARGET_UNLOCKED_FLAG - impl.refCounts[relHarmonic]);
}

// must be called every tick
void DependencyGraph::signalSource(SourceNodeImpl& impl, CompleteSource source, utick_t tick) {
    verifyValidTick(source.harmonic, tick);
    utick_t nextTick = tick + ((utick_t)1 << source.harmonic);
    impl.updates.insert(updateQueue(source, tick).take(nextTick));

    auto& graphUpdates = graphUpdateQueue(source.cluster(), tick);
    TickExpander expander = tickExpander(source.cluster());
    impl.updates.apply(
        [&](auto update) {
            auto dep = update.dep;
            int_t idx = impl.dependencies.find(dep);
            if (idx == impl.dependencies.size()) {
                impl.insert(dep);
                graphUpdates.pushBack(GraphUpdate::createDependency(source, dep));
            } else
                impl.dependencies[idx].refCount += dep.refCount;
        });
    impl.dependencies.removeIf([&](auto dep) {
        if (dep.refCount == 0) {
            graphUpdates.pushBack(GraphUpdate::removeDependency(source, dep));
            return true;
        }
        return false;
    });

    uint8_t includedHarmonics = std::countr_zero(tick);
    for (auto dep : impl.dependencies) {
        // tick is valid for a harmonic if and only if tick + dep.delay is
        if (dep.harmonic <= includedHarmonics)
            signalTarget(dep.target(source.cluster()), tick + dep.delay, dep.refCount);
    }
}

// must never be called after unlockTarget(..., target, sourceTick + ticksDelay)
void DependencyGraph::updateDependencyWithinCluster(CompleteSource source, CompleteTarget target, utick_t sourceUtick, tickdiff_t ticksDelay, refcnt_t delta) {
    verifyValidTick(source.harmonic, sourceUtick);
    verifyValidTick(target.harmonic, ticksDelay);
    int_t firstEmittingTick, firstReceivingTick;
    TickExpander expander = tickExpander(source.cluster());
    int_t sourceTick = expander.tick(sourceUtick);
    int_t relHarmonic = (int_t)target.harmonic - (int_t)source.harmonic;
    updateQueue(source, sourceUtick).pushBack({ { target, ticksDelay, delta }, sourceUtick }, [&](utick_t currentSourceUtick) {
        int_t currentSourceTick = expander.tick(currentSourceUtick);
        firstEmittingTick = std::max(currentSourceTick, sourceTick);
        firstReceivingTick = firstEmittingTick + ticksDelay;
        signal(target, firstReceivingTick).fetch_sub(delta, std::memory_order_relaxed);
    });
    updateQueue(target, firstReceivingTick).pushBack({ (uint8_t)std::max<int_t>(0, -relHarmonic), delta, (utick_t)firstReceivingTick });
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
    signalSource(impl.rootSourceImpl, rootSource(cluster), tick);
}

std::pair<CompleteSource, utick_t> DependencyGraph::maybeMapSource(CompleteSource source, Cluster cluster, utick_t sourceTick) {
    if (source.cluster() == cluster)
        return { source, sourceTick };

    // mapped sources are always in the 0th harmonic
    utick_t mappedSourceTick = translateTickCeil(source.cluster(), cluster, sourceTick);

    auto& impl = clusterImpl(cluster);
    auto [sourceMapping, wasAllocated] = impl.findOrCreateMapping(source, cluster, mappedSourceTick);
    if (!wasAllocated)
        return { sourceMapping, mappedSourceTick };

    updateDependencyWithinCluster(sourceMapping, retireTickTask().target(), mappedSourceTick, 0, 1);
    auto mappedTarget = clusterImpl(source.cluster()).createMappedTarget(source, sourceMapping, sourceTick);
    impl.registerMappedTarget(mappedTarget, mappedSourceTick);
    updateDependencyWithinCluster(source, mappedTarget, sourceTick, 0, 1);
    return { sourceMapping, mappedSourceTick };
}

/* ==============> Data <============== */

ClusterImpl& DependencyGraph::clusterImpl(Cluster cluster) {
    return m_clusterImpls[cluster.id];
}

MappedSourceNodeImpl& DependencyGraph::findMappedSource(CompleteTarget target, utick_t tick) {
    return clusterImpl(target.cluster()).findMappedSource(target, tick);
}

AtomicArrayBuilder<SourceUpdate>& DependencyGraph::updateQueue(CompleteSource source, utick_t tick) {
    return clusterImpl(source.cluster()).updateQueue(source, tick);
}

AtomicArrayBuilder<TargetUpdate>& DependencyGraph::updateQueue(CompleteTarget target, utick_t tick) {
    return clusterImpl(target.cluster()).updateQueue(target, tick);
}

/* ==============> ? <============== */

Task DependencyGraph::newTask(Promise* prom, Cluster cluster, uint8_t harmonic, utick_t tick);
    utick_t spawnTick = ceilTickToHarmonic(harmonic, tick);
    Task task = clusterImpl(cluster).createTask(prom, cluster, harmonic, spawnTick);
    updateDependencyWithinCluster(task.source(), retireTickTask(cluster).target(), spawnTick, 0, 1);
    return task;
}

void DependencyGraph::initializeCluster(Cluster cluster, int_t firstTick) {
    ClusterImpl& impl = clusterImpl(cluster);
    impl.initialize();

    // Don't call newTask() to avoid circular self dependence.
    Task createdRetireTickTask = impl.createTask(..., cluster, 0, firstTick);
    VERIFY(Task, createdRetireTickTask == retireTickTask(cluster));

    CompleteSource createdRootSource = impl.createPsudoMapping(cluster, firstTick);
    VERIFY(Task, createdRootSource == rootSource(cluster));
    updateDependencyWithinCluster(rootSource(cluster), retireTickTask(cluster).target(), firstTick, 0, 1);
}

/* ==============> ? <============== */

void DependencyGraph::retireClusterTick(RetireTaskImpl& impl, Cluster cluster, utick_t utick) {
    auto updates = graphUpdateQueue(cluster, utick).takeUnsafe();
    for (auto block : updates) {
        for (auto update : block) {
            if (!update.removal) {
                tickdiff_t effDelay = update.delay + ((tickdiff_t)1 << update.sourceHarmonic) - 1;
                if (effDelay > impl.maxDelay)
                    impl.maxDelay = effDelay;
            }
        }
    }
    deallocateInterfaces(cluster, utick);
    TickExpander expander = tickExpander(cluster);
    int_t tick = expander.tick(utick);
    int_t deallocateRangeBegin = impl.nextTickToDeallocate;
    int_t deallocateRangeEnd = std::max(deallocateRangeBegin, tick - impl.maxDelay + 1);
    for (int_t i = deallocateRangeBegin; i < deallocateRangeEnd; i++) {
        deallocateData(cluster, (utick_t)i);
    }
    impl.nextTickToDeallocate = deallocateRangeEnd;

}
