#pragma once

#include "containers/FlatSet.hpp"
#include "containers/IdAllocator.hpp"
#include "containers/RangeAllocator.hpp"
#include "containers/SmallPoolAllocator.hpp"
#include "task/task.hpp"
#include "util/uninitialized.hpp"
#include <memory>
#include <unordered_map>

template<>
struct std::hash<task::CompleteSource> {
    auto operator()(const task::CompleteSource& source) const {
        std::hash<uint32_t> h;
        return h(std::bit_cast<uint32_t>(source));
    }
};

namespace task {

template<typename T>
struct TickUpdateQueue : FlatSet<T> {
    template<typename Callback>
    void apply(Callback&& callback, utick_t utick, TickExpander expander) {
        auto tick = expander.tick(utick);
        this->removeIf([&](auto update) {
            if (expander.tick(update.startTick) <= tick) {
                callback(update);
                return true;
            }
            return false;
        });
    }

    /*template<typename Callback>
    void apply(Callback&& callback, utick_t utick, TickExpander expander) {
        auto tick = expander.tick(utick);
        for (auto update : *this) {
            if (expander.tick(update.startTick) <= tick) {
                callback(update);
            }
        }
    }*/
};

struct Dependency {
    uint16_t id = 0;
    uint8_t padding : CLUSTER_BITS = 0;
    uint8_t harmonic : HARMONIC_BITS = 0;
    bool mapping = false;
    tickdiff_t delay = 0;
    refcnt_t refCount = 0;

    Dependency(CompleteTarget target, tickdiff_t delay, refcnt_t refCount)
        : id(target.id)
        , harmonic(target.harmonic)
        , mapping(target.mapping)
        , delay(delay)
        , refCount(refCount) { }

    bool operator==(const Dependency& other) {
        return id == other.id
            && mapping == other.mapping
            && padding == other.padding
            && harmonic == other.harmonic
            && delay == other.delay;
    }

    CompleteTarget target(Cluster cluster) {
        return { {
            .id = id,
            .clusterId = cluster.id,
            .harmonic = harmonic,
            .mapping = mapping,
        } };
    }

    bool isMapping() { return mapping; }
};

struct SourceUpdate {
    Dependency dep;
    utick_t startTick = 0;
};

struct TargetUpdate {
    uint8_t relHarmonic = 0;
    refcnt_t refCount = 0;
    utick_t startTick = 0;
};

struct SourceNodeImpl {
    // stuff that depends on us
    FlatSet<Dependency> dependencies;
    TickUpdateQueue<SourceUpdate> updates;

    void clear() {
        dependencies.clear();
        updates.clear();
    }
};

struct TargetNodeImpl {
    // stuff we depend on
    urefcnt_t refCounts[NUM_HARMONICS] = {};
    TickUpdateQueue<TargetUpdate> updates;

    void clear() {
        for (int_t i = 0; i < NUM_HARMONICS; i++)
            refCounts[i] = 0;
        updates.clear();
    }
};

struct Promise {
    SourceNodeImpl sourceImpl;
    TargetNodeImpl targetImpl;
};

struct MappedSourceNodeImpl : SourceNodeImpl {
    CompleteSource source;
};

struct MappedTargetNodeImpl : TargetNodeImpl {
    CompleteTarget target;

    MappedTargetNodeImpl(CompleteTarget target)
        : target(target) { }
};

struct GraphUpdate {
    uint16_t sourceId = 0;
    uint8_t clusterId : CLUSTER_BITS = 0;
    uint8_t sourceHarmonic : HARMONIC_BITS = 0;
    uint16_t targetId = 0;
    bool removal : 1 = false;
    bool sourceMapping : 1 = false;
    bool targetMapping : 1 = false;
    uint8_t padding : (CLUSTER_BITS - 3) = 0;
    uint8_t targetHaromic : HARMONIC_BITS = 0;
    tickdiff_t delay = 0;

    static GraphUpdate update(bool removal, CompleteSource source, Dependency dep) {
        return {
            .sourceId = source.id,
            .clusterId = source.clusterId,
            .sourceHarmonic = source.harmonic,
            .targetId = dep.id,
            .removal = removal,
            .sourceMapping = source.isMapping(),
            .targetMapping = dep.isMapping(),
            .targetHaromic = dep.harmonic,
            .delay = dep.delay,
        };
    }

    static GraphUpdate removeDependency(CompleteSource source, Dependency dep) {
        return update(true, source, dep);
    }

    static GraphUpdate createDependency(CompleteSource source, Dependency dep) {
        return update(false, source, dep);
    }
};

struct RetireTaskImpl {
    tickdiff_t maxDelay = 0;
    int_t nextTickToFinalize = 0;
};

struct RetireTaskPromise : Promise{
    RetireTaskImpl retireImpl;
};

constexpr uint16_t FIRST_VALID_TASK_ID = 1;
constexpr uint16_t FIRST_VALID_SOURCE_MAPPING_ID = 1;
constexpr uint16_t FIRST_VALID_MAPPED_TARGET_ID = 1;

// system tasks
constexpr uint16_t RETIRE_TICK_TASK_ID = 1;

// system source mappings
constexpr uint16_t ROOT_SOURCE_ID = 1;
constexpr uint16_t DELAYED_ROOT_SOURCE_ID = 2;

constexpr uint16_t FIRST_USER_TASK_ID = 2;
constexpr uint16_t FIRST_USER_SOURCE_MAPPING_ID = 3;
constexpr uint16_t FIRST_USER_MAPPED_TARGET_ID = 1;

struct ClusterImpl {
    /* ==============> Task allocator <============== */
    std::mutex taskIdAllocatorLock;
    sIdAllocator taskIdAllocator;
    std::unique_ptr<Promise*[]> taskPromises;

    Task createTask(Promise*, Cluster, uint8_t harmonic, utick_t);

    /* ==============> Source mapping allocator <============== */
    std::shared_mutex sourceMappingLock;
    std::unordered_map<CompleteSource, CompleteSource> sourceMappings;
    sIdAllocator sourceMappingAllocator;

    // the caller must hold sourceMappingLock
    CompleteSource createPsudoMapping(Cluster, utick_t);
    // Get the mapping of source in cluster, where this impl corresponds cluster.
    std::pair<CompleteSource, bool> findOrCreateMapping(CompleteSource, Cluster, utick_t);

    /* ==============> Mapped target allocator <============== */
    std::mutex mappedTargetAllocatorLock;
    sIdAllocator mappedTargetAllocator;
    // The mapped sources in other clusters associated with mapped targets in this cluster.
    std::unique_ptr<MappedSourceNodeImpl[]> mappedSourceNodes;

    // Create the target associated to the mapping sourceMapping of source,
    // where this impl corresponds to source.cluster()
    CompleteTarget createMappedTarget(CompleteSource source, CompleteSource sourceMapping, utick_t);
    MappedSourceNodeImpl& findMappedSource(CompleteTarget, utick_t);

    /* ==============> Mapped target tracking <============== */
    // All the mapped targets in other clusters that are associated with
    // a source mapping into this cluster.
    FlatSet<CompleteTarget> mappedTargets;
    AtomicArrayBuilder<CompleteTarget> mappedTargetQueue;

    void registerMappedTarget(CompleteTarget target, utick_t) {
        mappedTargetQueue.pushBack(target);
    }
    FlatSet<CompleteTarget>& getMappedTargets(utick_t) {
        mappedTargets.insert(*mappedTargetQueue.take());
        return mappedTargets;
    }

    /* ==============> Update queues <============== */
    std::unique_ptr<AtomicArrayBuilder<SourceUpdate>[]> mappedSourceUpdateQueues;
    std::unique_ptr<AtomicArrayBuilder<TargetUpdate>[]> mappedTargetUpdateQueues;
    std::unique_ptr<AtomicArrayBuilder<SourceUpdate>[]> taskSourceUpdateQueues;
    std::unique_ptr<AtomicArrayBuilder<TargetUpdate>[]> taskTargetUpdateQueues;

    AtomicArrayBuilder<SourceUpdate>& updateQueue(CompleteSource source, utick_t) {
        return (source.isMapping() ? mappedSourceUpdateQueues : taskSourceUpdateQueues)[source.id];
    }
    AtomicArrayBuilder<TargetUpdate>& updateQueue(CompleteTarget target, utick_t) {
        VERIFY(Task, !target.isMapping());
        return taskTargetUpdateQueues[target.id];
    }

    /* ==============> System source mappings <============== */
    SourceNodeImpl rootSourceImpl;
    SourceNodeImpl delayedRootSourceImpl;

    /* ==============> Data heap <============== */
    std::mutex dataHeapPageLock;
    sIdAllocator dataHeapPageMap;
    FlatSet<utick_t> committedDataHeapTicks;
    std::mutex dataHeapAllocatorLock;
    RangeAllocator dataHeapAllocator;

    void commitDataPages(Cluster cluster, utick_t tick);
    void decommitDataPages(Cluster cluster, utick_t tick);
    std::pair<uint32_t, uint32_t> commitNewDataPage(Cluster cluster, utick_t allocUtick);

    /* ==============> Interface heap <============== */
    std::mutex interfaceHeapAllocatorLock;
    RangeAllocator interfaceHeapAllocator;

    /* ==============> Globals <============== */
    int_t m_mappedTargetSignalOffset;
    ClusterInitializaionBounds bounds;
    int_t nextTickToInitialize;
    utick_t currentTick;
    TickExpander tickExpander;
    int_t numInteractingClusters = 0;
    std::array<Cluster, 32> interactingClusters;
    RetireTaskPromise retireTaskPromise;

    int_t mappedTargetSignalOffset() { return m_mappedTargetSignalOffset; }
    int_t taskSignalOffset() { return sizeof(InterfaceHeapHeader); }

    ClusterImpl(Cluster cluster, int_t firstTick, ClusterInitializaionBounds bounds);
    ~ClusterImpl();
};

inline utick_t ceilTickToHarmonic(uint8_t harmonic, utick_t tick) {
    utick_t mask = ((utick_t)1 << harmonic) - 1;
    return (tick + mask) & ~mask;
}
inline utick_t floorTickToHarmonic(uint8_t harmonic, utick_t tick) {
    utick_t mask = ((utick_t)1 << harmonic) - 1;
    return tick & ~mask;
}
inline Address tickBase(Cluster cluster, utick_t tick) {
    return Address().withCluster(cluster).withTick(tick);
}

struct DependencyGraph {

    /* ==============> Static <============== */
    static Task retireTickTask(Cluster cluster) {
        return { .id = RETIRE_TICK_TASK_ID, .clusterId = cluster.id, .harmonic = 0 };
    }
    static CompleteTarget retireTickTarget(Cluster cluster) {
        return retireTickTask(cluster).target();
    }
    static CompleteSource rootSource(Cluster cluster) {
        return CompleteSource::mapping(ROOT_SOURCE_ID, cluster);
    }
    static CompleteSource delayedRootSource(Cluster cluster) {
        return CompleteSource::mapping(DELAYED_ROOT_SOURCE_ID, cluster);
    }

    /* ==============>  Data structure <============== */
    uninitialized<ClusterImpl> m_clusterImpls[32];

    ClusterImpl& clusterImpl(Cluster cluster) {
        return m_clusterImpls[cluster.id].value;
    }
    MappedSourceNodeImpl& findMappedSource(CompleteTarget target, utick_t tick) {
        return clusterImpl(target.cluster()).findMappedSource(target, tick);
    }
    AtomicArrayBuilder<SourceUpdate>& updateQueue(CompleteSource source, utick_t tick) {
        return clusterImpl(source.cluster()).updateQueue(source, tick);
    }
    AtomicArrayBuilder<TargetUpdate>& updateQueue(CompleteTarget target, utick_t tick) {
        return clusterImpl(target.cluster()).updateQueue(target, tick);
    }
    ArrayView<const Cluster> interactingClusters(Cluster cluster) {
        auto& impl = clusterImpl(cluster);
        return { impl.interactingClusters.data(), impl.numInteractingClusters };
    }
    TickExpander tickExpander(Cluster cluster) {
        return clusterImpl(cluster).tickExpander;
    }

    DataHeapHeader& dataHeapHeader(Cluster cluster, utick_t tick) {
        return *(DataHeapHeader*)tickBase(cluster, tick).withAddr(PERSISTENT_DATA_HEAP_BEGIN).ptr();
    }
    InterfaceHeapHeader& interfaceHeapHeader(Cluster cluster, utick_t tick) {
        return *(InterfaceHeapHeader*)tickBase(cluster, tick).withAddr(PERSISTENT_INTERFACE_HEAP_BEGIN).ptr();
    }

    std::atomic<urefcnt_t>& signal(CompleteTarget target, utick_t tick) {
        auto& impl = clusterImpl(target.cluster());
        Address addr = tickBase(target.cluster(), tick).withAddr(PERSISTENT_INTERFACE_HEAP_BEGIN);

        int_t taskOffset = impl.taskSignalOffset();
        int_t mappedTargetOffset = impl.mappedTargetSignalOffset();
        int_t offset = target.mapping ? mappedTargetOffset : taskOffset;
        addr.addToAddr(offset);

        return ((std::atomic<urefcnt_t>*)addr.ptr())[target.id];
    }
    HalfAtomicArrayBuilder<GraphUpdate, InterfaceHeapBumpAllocator>& graphUpdateQueue(Cluster cluster, utick_t tick) {
        return dataHeapHeader(cluster, tick).graphUpdates;
    }
    HalfAtomicArrayBuilder<HeapAllocation, InterfaceHeapBumpAllocator>& freedDataHeapAllocations(Cluster cluster, utick_t tick) {
        return dataHeapHeader(cluster, tick).freedAllocations;
    }
    HalfAtomicArrayBuilder<HeapAllocation, InterfaceHeapBumpAllocator>& freedInterfaceHeapAllocations(Cluster cluster, utick_t tick) {
        return interfaceHeapHeader(cluster, tick).freedAllocations;
    }
    ExternalTicks& externalTicks(Cluster cluster, utick_t tick) {
        return dataHeapHeader(cluster, tick).ticks;
    }
    std::atomic<int8_t>& clusterDataRefCount(Cluster cluster, utick_t tick) {
        return dataHeapHeader(cluster, tick).refCount;
    }

    /* ==============> TODO <============== */
    void scheduleTask(Task, utick_t);

    /* ==============> Concurrency <============== */
    static constexpr urefcnt_t TARGET_UNLOCKED_FLAG = (urefcnt_t)1 << (std::numeric_limits<urefcnt_t>::digits - 1);
    void signalTarget(CompleteTarget target, utick_t tick, urefcnt_t delta);
    void unlockMappedTarget(CompleteTarget target, utick_t tick);
    void unlockNextTarget(TargetNodeImpl& impl, CompleteTarget target, utick_t tick);
    void signalSource(SourceNodeImpl& impl, CompleteSource source, utick_t tick);
    void updateDependencyWithinCluster(CompleteSource source, CompleteTarget target, utick_t sourceUtick, tickdiff_t ticksDelay, refcnt_t delta);

    /* ==============> External handling and Tick lifecycle <============== */
    CompleteSource updateDependency(CompleteSource source, CompleteTarget target, utick_t sourceTick, tickdiff_t targetTicksDelay, refcnt_t delta);
    void onDepedenciesFulfilled(CompleteTarget target, utick_t tick);
    std::pair<CompleteSource, utick_t> maybeMapSource(CompleteSource source, Cluster cluster, utick_t sourceTick);
    void initializeClusterTick(Cluster cluster, utick_t tick);
    void beginClusterTicks(ArrayView<const Cluster> incrementClusters);
    void retireClusterTick(RetireTaskImpl& impl, Cluster cluster, utick_t utick);
    utick_t translateTick(Cluster from, Cluster to, utick_t tick);
    utick_t translateTickFloor(Cluster from, Cluster to, utick_t tick);
    utick_t translateTickCeil(Cluster from, Cluster to, utick_t tick);
    bool isCarringDependence(Cluster from, Cluster to, utick_t tick);

    /* ==============> ? <============== */
    std::mutex clusterIdFreeMaskLock;
    uint32_t clusterIdFreeMask = std::numeric_limits<uint32_t>::max();

    void globalInitialize();
    void globalFinalize();

    Task newTask(Promise* prom, Cluster cluster, uint8_t harmonic, utick_t tick);
    void taskCompleted(Promise& prom, Task task, utick_t tick);
    Cluster newCluster(int_t firstTick, ClusterInitializaionBounds bounds);

    /* ==============> ? <============== */
    uint32_t allocateData(Cluster cluster, utick_t tick, size_t alignment, int_t size);
    uint32_t allocateInterface(Cluster cluster, utick_t tick, size_t alignment, int_t size);
    void initializeClusterData(Cluster cluster, utick_t tick);
    void finalizeClusterData(Cluster cluster, utick_t tick);
    void initializeClusterInterfaces(Cluster cluster, utick_t tick);
    void finalizeClusterInterfaces(Cluster cluster, utick_t tick);
    void refClusterData(Cluster cluster, utick_t tick);
    void unrefClusterData(Cluster cluster, utick_t tick);
};

}
