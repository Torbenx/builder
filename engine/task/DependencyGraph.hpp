#pragma once

namespace task {

template<typename T>
class UpdateQueue : public FlatSet<T> {
public:
    template<typename Callback, typename Condition>
    void apply(Callback&& callback, Condition&& cond) {
        removeIf([](auto update) {
            if (cond(update)) {
                callback(update);
                return true;
            }
            return false;
        });
    }
};

template<typename T>
class TickUpdateQueue : public UpdateQueue<T> {
    template<typename Callback>
    void apply(Callback&& callback, utick_t utick, TickExpander expander) {
        auto tick = expander.tick(utick);
        apply(callback,
            [&](auto update) {
                return expander.tick(update.startTick) <= tick;
            });
    }
};

using utick_t = uint32_t;
using urefcnt_t = uint16_t;
using refcnt_t = std::make_signed_t<urefcnt_t>;
using tickdiff_t = int16_t;

constexpr size_t HARMONICS_BITS = 3;
constexpr size_t NUM_HARMONICS = (size_t)1 << HARMONICS_BITS;
constexpr size_t CLUSTER_BITS = 8 - HARMONICS_BITS;
constexpr size_t MAX_GROUPS = 256;

struct Cluster {
    uint8_t id = 0;
};

struct CompleteNode {
    uint16_t id = 0;
    uint8_t clusterId : CLUSTER_BITS = 0;
    uint8_t harmonic : HARMONICS_BITS = 0;
    bool mapping = false;

    Cluster cluster() { return { clusterId }; }
    bool isMapping() { return mapping; }
};

struct CompleteSource : CompleteNode {
    static CompleteSource mapping(uint16_t id, Cluster cluster) {
        return { {
            .id = id,
            .clusterId = cluster.id,
            .harmonic = 0,
            .mapping = true,
        } };
    }
};

struct CompleteTarget : CompleteNode {
    static CompleteTarget mapped(uint16_t id, CompleteSource source) {
        return { {
            .id = id,
            .clusterId = source.clusterId,
            .harmonic = source.harmonic,
            .mapping = true,
        } };
    }
};

struct Task {
    uint16_t id = 0;
    uint8_t clusterId : CLUSTER_BITS = 0;
    uint8_t harmonic : HERMONIC_BITS = 0;

    Cluster cluster() { return { clusterId }; }
    CompleteSource source() {
        return { {
            .id = id,
            .clusterId = clusterId,
            .harmonic = harmonic,
            .mapping = false,
        } };
    }
    CompleteTarget target() {
        return { {
            .id = id,
            .clusterId = clusterId,
            .harmonic = harmonic,
            .mapping = false,
        } };
    }
};

struct Dependency {
    uint16_t id = 0;
    uint8_t padding : CLUSTER_BITS = 0;
    uint8_t harmonic : HARMONICS_BITS = 0;
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
            .mapping = mapping,
            .clusterId = cluster.id,
            .harmonic = harmonic,
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

struct MappedSourceNodeImpl : SourceNodeImpl {
    CompleteSource source;
};

struct GraphUpdate {
    uint16_t sourceId = 0;
    uint8_t clusterId : CLUSTER_BITS = 0;
    uint8_t sourceHarmonic : HARMONICS_BITS = 0;
    uint16_t targetId = 0;
    bool removal : 1 = false;
    bool sourceMapping : 1 = false;
    bool targetMapping : 1 = false;
    uint8_t padding : (CLUSTER_BITS - 3) = 0;
    uint8_t targetHaromic : HARMONICS_BITS = 0;
    tickdiff_t delay = 0;

    static GraphUpdate update(bool removal, CompleteSource source, Dependency dep) {
        return {
            .removal = removal,
            .sourceId = source.id,
            .clusterId = source.clusterId,
            .sourceHarmonic = source.harmonic,
            .sourceMapping = source.isMapping(),
            .targetId = dep.id,
            .targetHaromic = dep.harmonic,
            .targetMapping = dep.isMapping(),
            .delay = dep.delay,
        };
    }

    static GraphUpdate removeDependency(CompleteSource source, Dependency dep) {
        return update(true, source, target);
    }

    static GraphUpdate createDependency(CompleteSource source, Dependency dep) {
        return update(false, source, target);
    }
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
    // The mapped sources in other clusters associated with mappedTargets in this cluster.
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
        mappedTargets.insert(mappedTargetQueue.take());
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
        return (target.isMapping() ? mappedTargetUpdateQueues : taskTargetUpdateQueues)[target.id];
    }

    /* ==============> System source mappings <============== */
    SourceNodeImpl rootSourceImpl;
    SourceNodeImpl delayedRootSourceImpl;

    /* ==============> initializeClusterTick <============== */
    int_t nextTickToAllocate;
    int_t maxInterfaceTicksDelay;

    /* ==============> Globals <============== */
    void ClusterImpl(int_t firstTick, int_t maxTasks, int_t maxMappedTargets, int_t maxSourceMappings, int_t maxInterfaceTicksDelay);
    ~ClusterImpl();
};

struct RetireTaskImpl {
    tickdiff_t maxDelay = 0;
    int_t nextTickToDeallocate = 0;
};

struct ExternalTicks {
    std::bitmask<32> dependenceMask = 0;
    utick_t ticks[32] {};

    utick_t tick(Cluster cluster) const {
        return ticks[cluster.id];
    }

    void setTick(Cluster cluster, utick_t tick, bool carriesDependence) {
        ticks[cluster.id] = tick;
        dependenceMask.set(cluster.id, carriesDependence);
    }

    bool carriesDependence(Cluster cluster) const {
        return dependenceMask.test(cluster.id);
    }
};

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
    static utick_t ceilTickToHarmonic(uint8_t harmonic, utick_t tick) {
        utick_t mask = ((utick_t)1 << harmonic) - 1;
        return (tick + mask) & ~mask;
    }
    static utick_t floorTickToHarmonic(uint8_t harmonic, utick_t tick) {
        utick_t mask = ((utick_t)1 << harmonic) - 1;
        return tick & ~mask;
    }

    /* ==============>  Data structure <============== */
    uninitialized<ClusterImpl> m_clusterImpls[32];

    ClusterImpl& clusterImpl(Cluster cluster);
    AtomicArrayBuilder<SourceUpdate>& updateQueue(CompleteSource);
    AtomicArrayBuilder<TargetUpdate>& updateQueue(CompleteTarget);
    MappedSourceNodeImpl& findMappedSource(CompleteTarget, utick_t);
    utick_t currentTick(Cluster);
    ArrayView<const Cluster> interactingClusters(Cluster);

    std::atomic<urefcnt_t>& signal(CompleteTarget, utick_t);
    HalfAtomicArrayBuilder<GraphUpdate>& graphUpdateQueue(Cluster, utick_t);
    ExternalTicks& externalTicks(Cluster, utick_t);
    std::atomic<int8_t>& clusterDataRefCount(Cluster, utick_t);

    /* ==============> TODO <============== */

    TickExpander tickExpander(Cluster);
    // Translating using this method is only valid if the tick is carring dependence.
    utick_t translateTick(Cluster from, Cluster to, utick_t);
    utick_t translateTickCeil(Cluster from, Cluster to, utick_t);
    utick_t translateTickFloor(Cluster from, Cluster to, utick_t);
    bool isCarringDependence(Cluster from, Cluster to, utick_t);

    void scheduleTask(Task, utick_t);

    /* ==============> Concurrency <============== */
    static constexpr urefcnt_t TARGET_UNLOCKED_FLAG = (urefcnt_t)1 << (std::numeric_limits<urefcnt_t>::digits - 1);

    void signalTarget(CompleteTarget, utick_t, urefcnt_t);
    void unlockTarget(TargetNodeImpl&, CompleteTarget, utick_t);
    void signalSource(SourceNodeImpl&, CompleteSource, utick_t);
    void updateDependencyWithinCluster(CompleteSource, CompleteTarget, utick_t sourceTick, tickdiff_t ticksDelay, refcnt_t delta);

    /* ==============> ? <============== */
    CompleteSource updateDependency(CompleteSource source, CompleteTarget target, utick_t sourceTick, tickdiff_t targetTicksDelay, refcnt_t delta);
    void onDepedenciesFulfilled(CompleteTarget, utick_t);
    Target updateDependency(Task source, Task target, utick_t sourceTick, tickdiff_t sourceTicksDelay, refcnt_t delta);
    void beginClusterTick(Cluster cluster, utick_t tick, const std::bitset<MAX_GROUPS>& firstTimeUsingTick);
    CompleteSource maybeMapSource(Cluster, CompleteSource, utick_t);

    /* ==============> ? <============== */
    std::mutex clusterIdFreeMaskLock;
    uint32_t clusterIdFreeMask;

    Task newTask(Promise* prom, Cluster, uint8_t harmonic, utick_t);
    Cluster newCluster(int_t firstTick);
};

}
