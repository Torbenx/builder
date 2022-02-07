#include "task/DependencyGraph.hpp"

namespace task {

static void verifyValidTick(uint8_t harmonic, utick_t tick) {
    VERIFY(Task, tick & (((utick_t)1 << harmonic) - 1) == 0);
}

/* ------------------------------------------- *
 * ==============> ClusterImpl <============== *
 * ------------------------------------------- */

/* ==============> Task allocator <============== */

Task ClusterImpl::createTask(Promise* promise, Cluster cluster, uint8_t harmonic, utick_t) {
    std::unique_lock g(taskIdAllocatorLock);
    auto id = taskIdAllocator.allocateId();
    taskPromises[id] = promise;
    return { id, cluster.id, harmonic };
}

/* ==============> Source mapping allocator <============== */

CompleteSource ClusterImpl::createPsudoMapping(Cluster cluster, utick_t) {
    auto id = sourceMappingAllocator.allocateId();
    return CompleteSource::mapping(id, cluster);
}

std::pair<CompleteSource, bool> ClusterImpl::findOrCreateMapping(CompleteSource source, Cluster cluster, utick_t tick) {
    {
        std::shared_lock g(sourceMappingLock);
        auto it = sourceMappings.find(source);
        if (it != sourceMappings.end())
            return { it->second, false };
    }
    std::unique_lock g(sourceMappingLock);
    auto mapping = createPsudoMapping(cluster, tick);
    sourceMappings.insert({ source, mapping });
    return { mapping, true };
}

/* ==============> Mapped target allocator <============== */

CompleteTarget ClusterImpl::createMappedTarget(CompleteSource source, CompleteSource sourceMapping, utick_t) {
    std::unique_lock g(mappedTargetAllocatorLock);
    auto id = mappedTargetAllocator.allocateId();
    mappedSourceNodes[id].source = sourceMapping;
    return CompleteTarget::mapped(id, source);
}

MappedSourceNodeImpl& ClusterImpl::findMappedSource(CompleteTarget target, utick_t) {
    VERIFY(Task, target.isMapping());
    return mappedSourceNodes[target.id];
}

/* ==============> Data heap <============== */

void ClusterImpl::commitDataPages(Cluster cluster, utick_t tick) {
    std::unique_lock g(dataHeapPageLock);
    Address base = tickBase(cluster, tick).withAddr(PERSISTENT_DATA_HEAP_BEGIN);
    auto pageSize = platform::pageSize();
    dataHeapPageMap.enumerateUsedIdRanges([&](uint16_t begin, size_t size)) {
        platform::commitMemory(base.addToAddr(begin * pageSize).ptr(), size * pageSize);
    }
    committedDataHeapTicks.insert(tick);
}

void ClusterImpl::decommitDataPages(Cluster cluster, utick_t tick) {
    std::unique_lock g(dataHeapPageLock);
    platform::decommitMemory(tickBase(cluster, tick).withAddr(DATA_HEAP_BEGIN).ptr(), DATA_HEAP_SIZE);
    int_t index = committedDataHeapTicks.find(tick);
    VERIFY(Task, index != committedDataHeapTicks.size());
    committedDataHeapTicks.removeAt(index);
}

std::pair<uint32_t, uint32_t> ClusterImpl::commitNewDataPage(Cluster cluster, utick_t allocUtick) {
    std::unique_lock g(dataHeapPageLock);
    uint16_t page = dataHeapPageMap.allocateId();
    auto pageSize = platform::pageSize();
    uint32_t addr = PERSISTENT_DATA_HEAP_BEGIN + page * pageSize;
    Address address = Address().withCluster(cluster).withAddr(addr);
    auto expander = tickExpander(cluster);
    int_t allocTick = expander.tick(allocUtick);
    for (utick_t utick : committedDataHeapTicks) {
        int_t tick = expander.tick(utick);
        if (tick >= allocTick)
            platform::commitMemory(address.withTick(utick).ptr(), pageSize);
    }
    return { addr, pageSize };
}

/* ==============> Globals <============== */

ClusterImpl::ClusterImpl(int_t firstTick, ClusterInitializaionBounds bounds)
    : mappedSourceNodes(new MappedSourceNodeImpl[bounds.maxMappedTargets])
    , mappedSourceUpdateQueues(new AtomicArrayBuilder<SourceUpdate>[bounds.maxSourceMappings])
    , mappedTargetUpdateQueues(new AtomicArrayBuilder<TargetUpdate>[bounds.maxMappedTargets])
    , taskSourceUpdateQueues(new AtomicArrayBuilder<SourceUpdate>[bounds.maxTasks])
    , taskTargetUpdateQueues(new AtomicArrayBuilder<TargetUpdate>[bounds.maxTasks])
    , taskPromises(new Promise*[bounds.maxTasks]())
    , taskIdAllocator(FIRST_VALID_TASK_ID)
    , sourceMappingAllocator(FIRST_VALID_SOURCE_MAPPING_ID)
    , mappedTargetAllocator(FIRST_VALID_MAPPED_TARGET_ID)
    , bounds(bounds) {

    interfaceHeapAllocator.expandRange(bounds.totalInfaceHeapSize());
    int_t taskSignalOffset = sizeof(InterfaceHeapHeader);
    m_mappedTargetSignalOffset = taskSignalOffset + maxTasks * sizeof(urefcnt_t);
    int_t dynamicAllocationOffset = m_mappedTargetSignalOffset + maxMappedTargets * sizeof(urefcnt_t);
    uint32_t allocatedInterfaceHeapHeaderOffset = interfaceHeapAllocator.allocate(1, dynamicAllocationOffset);
    EXPECT(Task, allocatedInterfaceHeapHeaderOffset, 0);

    [[maybe_unused]] auto [offset, allocSize] = commitNewDataPage(cluster, /* doesn't matter */ 0);
    VERIFY(Task, sizeof(DataHeapHeader) < allocSize));
    dataHeapAllocator.expandRange(allocSize);
    uint32_t allocatedDataHeapHeaderOffset = dataHeapAllocator.allocate(1, sizeof(DataHeapHeader));
    EXPECT(Task, allocatedDataHeapHeaderOffset, 0);
}

ClusterImpl::~ClusterImpl() { }

/* ----------------------------------------------- *
 * ==============> DependencyGraph <============== *
 * ----------------------------------------------- */

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

CompleteSource DependencyGraph::updateDependency(CompleteSource source, CompleteTarget target, utick_t sourceTick, tickdiff_t targetTicksDelay, refcnt_t delta) {
    verifyValidTick(source.harmonic, sourceTick);
    verifyValidTick(target.harmonic, targetTicksDelay);
    auto [mappedSource, mappedSourceTick] = maybeMapSource(source, target.cluster(), sourceTick);
    updateDependencyWithinCluster(mappedSource, target, mappedSourceTick, targetTicksDelay, delta);
    return mappedSource;
}

void DependencyGraph::onDepedenciesFulfilled(CompleteTarget target, utick_t tick) {
    verifyValidTick(target.harmonic, tick);
    if (target.isMapping()) {
        auto& impl = findMappedSource(target, tick);
        // we only get here after unlockTarget in beginClusterTick().
        utick_t sourceTick = translateTickCeil(target.cluster(), impl.source.cluster(), tick);
        signalSource(impl, impl.source, sourceTick);
    } else {
        scheduleTask(target.asTask(), tick);
    }
}

/* ==============> External handling and Tick lifecycle <============== */

/*
 * Tasks providing data to external tasks don't depend on anything.
 * Tasks receiving data from external tasks depend on their root,
   since they must know from which sourceTick to take it.
 * Tasks holding an interface to an external task depend on their root,
   since they must know where to write messages.
 * Tasks providing an interface to an external task depend on their root,
   since they must know which sourceTick to depend on.
 */

std::pair<CompleteSource, utick_t> DependencyGraph::maybeMapSource(CompleteSource source, Cluster cluster, utick_t sourceTick) {
    if (source.cluster() == cluster)
        return { source, sourceTick };

    // mapped sources are always in the 0th harmonic
    utick_t mappedSourceTick = translateTickCeil(source.cluster(), cluster, sourceTick);

    auto& impl = clusterImpl(cluster);
    auto [sourceMapping, wasAllocated] = impl.findOrCreateMapping(source, cluster, mappedSourceTick);
    if (!wasAllocated)
        return { sourceMapping, mappedSourceTick };

    updateDependencyWithinCluster(sourceMapping, retireTickTarget(cluster), mappedSourceTick, 0, 1); // is this necessary?
    auto mappedTarget = clusterImpl(source.cluster()).createMappedTarget(source, sourceMapping, sourceTick);
    impl.registerMappedTarget(mappedTarget, mappedSourceTick);
    updateDependencyWithinCluster(source, mappedTarget, sourceTick, 0, 1);
    return { sourceMapping, mappedSourceTick };
}

void DependencyGraph::initializeClusterTick(Cluster cluster, utick_t tick) {
    initializeClusterData(cluster, tick);
    initializeClusterInterfaces(cluster, tick + impl.bounds.maxInterfaceTicksDelay);
}

static bool containsBitFlip(utick_t a, utick_t b, int bit) {
    return (a ^ b) & -((utick_t)1 << bit);
}

void DependencyGraph::beginClusterTick(Cluster cluster, utick_t tick) {
    auto& impl = clusterImpl(cluster);

    auto& prevExtTicks = externalTicks(cluster, tick - 1);
    auto& currentExtTicks = externalTicks(cluster, tick);

    for (Cluster otherCluster : interactingClusters(cluster)) {
        utick_t prevTick = prevExtTicks.tick(otherCluster);
        utick_t currentTick = currentTick(otherCluster);
        bool dependence = prevTick != currentTick;
        currentExtTicks.setTick(otherCluster, currentTick, dependence);
        if (dependence)
            refClusterData(otherCluster, currentTick);
    }

    signalSource(impl.delayedRootSourceImpl, delayedRootSource(cluster), tick - 1);
    for (auto mappedTarget : impl.getMappedTargets()) {
        utick_t prevTargetTick = prevExtTicks.tick(mappedTarget.cluster());
        utick_t currentTragetTick = currentExtTicks.tick(mappedTarget.cluster());
        bool shouldDepend = containsBitFlip(prevTargetTick, currentTragetTick, mappedTarget.harmonic);
        if (shouldDepend) {
            utick_t targetTickToDependOn = floorTickToHarmonic(mappedTarget.harmonic, currentTragetTick);
            EXPECT(Task, translateTickCeil(mappedTarget.cluster(), cluster, targetTickToDependOn), tick); // this used for the inverse tick mapping
            unlockTarget(mappedTarget, targetTickToDependOn, 1);
        } else {
            auto& impl = findMappedSource(mappedTarget, currentTragetTick);
            utick_t sourceTick = tick; // mapped sources are always in the 0th harmonic
            signalSource(impl, impl.source, sourceTick);
        }
    }
    signalSource(impl.rootSourceImpl, rootSource(cluster), tick);
}

void DependencyGraph::retireClusterTick(RetireTaskImpl& impl, Cluster cluster, utick_t utick) {
    auto& clusterImpl = clusterImpl(cluster);

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

    {
        std::unique_lock g(clusterImpl.dataHeapAllocatorLock);
        auto allocations = freedDataHeapAllocations(cluster, utick).takeUnsafe();
        for (auto block : allocations) {
            for (auto allocation : block)
                clusterImpl.dataHeapAllocator.free(allocation.offset);
        }
    }
    {
        std::unique_lock g(clusterImpl.interfaceHeapAllocatorLock);
        auto allocations = freedInterfaceHeapAllocations(cluster, utick).takeUnsafe();
        for (auto block : allocations) {
            for (auto allocation : block)
                clusterImpl.interfaceHeapAllocator.free(allocation.offset);
        }
    }

    finalizeClusterInterfaces(cluster, utick);
    TickExpander expander = tickExpander(cluster);
    int_t tick = expander.tick(utick);
    int_t finalizeRangeBegin = impl.nextTickToFinalize;
    int_t finalizeRangeEnd = std::max(finalizeRangeBegin, tick - impl.maxDelay + 1);
    for (int_t i = finalizeRangeBegin; i < finalizeRangeEnd; i++) {
        finalizeClusterData(impl, cluster, (utick_t)i);
    }
    impl.nextTickToFinalize = finalizeRangeEnd;
}

utick_t DependencyGraph::translateTick(Cluster from, Cluster to, utick_t tick) {
    auto& extTicks = externalTicks(from, tick);
    VERIFY(Task, extTicks.carriesDependence(to));
    return extTicks.tick(to);
}

utick_t DependencyGraph::translateTickFloor(Cluster from, Cluster to, utick_t tick) {
    auto& extTicks = externalTicks(from, tick);
    return extTicks.tick(to);
}

utick_t DependencyGraph::translateTickCeil(Cluster from, Cluster to, utick_t tick) {
    auto& extTicks = externalTicks(from, tick);
    utick_t ret = extTicks.tick(to);
    return extTicks.carriesDependence(to) ? ret : ret + 1;
}

bool DependencyGraph::isCarringDependence(Cluster from, Cluster to, utick_t tick) {
    auto& extTicks = externalTicks(from, tick);
    return extTicks.carriesDependence(to);
}

/* ==============> ? <============== */

Task DependencyGraph::newTask(Promise* prom, Cluster cluster, uint8_t harmonic, utick_t tick) {
    verifyValidTick(harmonic, tick);
    Task task = clusterImpl(cluster).createTask(prom, cluster, harmonic, tick);
    updateDependencyWithinCluster(task.source(), retireTickTarget(cluster), tick, 0, 1);
    return task;
}

Cluster DependencyGraph::newCluster(int_t firstTick, ClusterInitializaionBounds bounds) {
    Cluster cluster;
    {
        std::unique_lock g(clusterIdFreeMaskLock);
        auto [id, mask] = SmallPoolAllocator<uint32_t>::allocate<1, 1>(clusterIdFreeMask);
        clusterIdFreeMask &= ~mask;
        cluster.id = id;
    }

    ClusterImpl& impl = clusterImpl(cluster);
    new (&impl) ClusterImpl(firstTick, bounds);

    // Don't call newTask() to avoid circular self dependence.
    Task createdRetireTickTask = impl.createTask(..., cluster, 0, firstTick);
    EXPECT(Task, createdRetireTickTask, retireTickTask(cluster));

    CompleteSource createdRootSource = impl.createPsudoMapping(cluster, firstTick);
    EXPECT(Task, createdRootSource, rootSource(cluster));
    updateDependencyWithinCluster(rootSource(cluster), retireTickTarget(cluster), firstTick, 0, 1);

    CompleteSource createdDelayedRootSource = impl.createPsudoMapping(cluster, firstTick);
    EXPECT(Task, createdDelayedRootSource, delayedRootSource(cluster));
    updateDependencyWithinCluster(delayedRootSource(cluster), retireTickTarget(cluster), firstTick, 0, 1);

    return cluster;
}

/* ==============> ? <============== */

uint32_t DependencyGraph::allocateData(Cluster cluster, utick_t tick, size_t alignment, int_t size) {
    auto& impl = clusterImpl(cluster);
    std::unique_lock g(impl.dataHeapAllocatorLock);
    return impl.dataHeapAllocator.allocate(alignment, size);
}

uint32_t DependencyGraph::allocateInterface(Cluster cluster, utick_t tick, size_t alignment, int_t size) {
    auto& impl = clusterImpl(cluster);
    std::unique_lock g(impl.interfaceHeapAllocatorLock);
    return impl.interfaceHeapAllocator.allocate(alignment, size);
}

void DependencyGraph::initializeClusterData(Cluster cluster, utick_t tick) {
    clusterImpl(cluster).commitDataPages(cluster, tick);
}

void DependencyGraph::finalizeClusterData(Cluster cluster, utick_t tick) {
    for (Cluster otherCluster : interactingClusters(cluster)) {
        // deallocateData(tick) always happens after beginClusterTick(tick + 1)
        if (isCarringDependence(cluster, otherCluster, tick + 1))
            unrefClusterData(otherCluster, translateTickFloor(cluster, otherCluster, tick));
    }
    unrefClusterData(cluster, tick);
}

void DependencyGraph::initializeClusterInterfaces(Cluster cluster, utick_t tick) {
    int_t interfaceHeapBytes = alignmentCeil(cluster.bounds.totalInfaceHeapSize(), platform::pageSize());
    platform::commitMemory(tickBase(cluster, tick).withAddr(PERSISTENT_INTERFACE_HEAP_BEGIN).ptr(), interfaceHeapBytes);
    auto& header = interfaceHeapHeader(cluster, tick);
    // ensure we have a zero page
    header.bumpAllocatorOffset = platform::pageSize();
    header.bumpAllocatorPageOffset = 1;
}

void DependencyGraph::finalizeClusterInterfaces(Cluster cluster, utick_t tick) {
    platform::decommitMemory(tickBase(cluster, tick).withAddr(INTERFACE_HEAP_BEGIN).ptr(), INTERFACE_HEAP_SIZE);
}

void DependencyGraph::refClusterData(Cluster cluster, utick_t tick) {
    auto oldCount = clusterDataRefCount(cluster, tick).fetch_add(1, std::memory_order_relaxed);
    VERIFY(Task, oldCount > 0);
}

void DependencyGraph::unrefClusterData(Cluster cluster, utick_t tick) {
    auto& impl = clusterImpl(cluster);
    auto oldCount = clusterDataRefCount(cluster, tick).fetch_sub(1, std::memory_order_relaxed);
    VERIFY(Task, oldCount > 0);
    auto newCount = oldCount - 1;
    if (newCount == 0)
        impl.decommitDataPages(cluster, tick);
}
