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
            [=](auto update) {
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

struct Task {
    uint32_t id : 24 = 0;
    uint32_t m_cluster : CLUSTER_BITS = 0;
    uint32_t harmonic : HERMONIC_BITS = 0;

    Cluster cluster() { return m_cluster; }
};

struct CompleteNode {
    uint32_t id : 23 = 0;
    bool mapping : 1 = false;
    uint32_t m_cluster : CLUSTER_BITS = 0;
    uint32_t harmonic : HARMONICS_BITS = 0;

    Cluster cluster() { return m_cluster; }
    bool isMapping() { return mapping; }
};

struct CompleteTarget : CompleteNode {
    static CompleteTarget mapped(uint32_t id, CompleteSource source) {
        return { id, true, source.cluster().id, source.harmonic };
    }
};

struct CompleteSource : CompleteNode {
    static CompleteSource mapping(uint32_t id, Cluster cluster) {
        return { id, true, cluster.id, 0 };
    }
};

struct Dependency {
    uint32_t id : 23 = 0;
    bool mapping : 1 = false;
    uint32_t padding : CLUSTER_BITS = 0;
    uint32_t absHarmonic : HARMONICS_BITS = 0;
    tickdiff_t delay = 0;
    refcnt_t refCount = 0;

    Dependency(CompleteTarget target, tickdiff_t delay, refcnt_t refCount)
        : id(target.id)
        , mapping(target.mapping)
        , absHarmonic(target.harmonic)
        , delay(delay)
        , refCount(refCount) { }

    bool operator==(const Dependency& other) {
        return id == other.id
            && mapping == other.mapping
            && padding == other.padding
            && absHarmonic == other.absHarmonic
            && delay == other.delay;
    }

    CompleteTarget target(Cluster cluster) {
        return { id, mapping, cluster.id, absHarmonic };
    }
};

struct SourceUpdate {
    Dependency dep;
    utick_t startTick = 0;
};

struct TargetUpdate {
    union {
        struct {
            bool dependencyUpdate = true;
            uint8_t relHarmonic = 0;
            refcnt_t refCount = 0;
        } dependency;

        struct {
            bool dependencyUpdate = false;
        } metadata;
    };
    utick_t startTick = 0;

    TargetUpdate(uint8_t relHarmonic, refcnt_t refCount, utick_t startTick)
        : dependency { .relHarmonic = relHarmonic, .refCount = refCount }, startTick(startTick) { }

    bool isDependencyUpdate() {
        return dependency.dependencyUpdate;
    }
};

struct SourceNodeImpl {
    // stuff that depends on us
    FlatSet<Dependency> dependencies;
    TickUpdateQueue<SourceUpdate> updates;
};

struct TargetNodeImpl {
    // stuff we depend on
    urefcnt_t refCount[NUM_HARMONICS] = {};
    TickUpdateQueue<TargetUpdate> updates;
};

struct MappedSourceNodeImpl : SourceNodeImpl {
    CompleteSource source;
};

struct DependencyGraph {

    /* ==============> Data <============== */
    struct ClusterImpl {
        std::shared_mutex sourceMappingLock;
        std::unordered_map<CompleteSource, CompleteSource> sourceMappings;
        IdAllocator<uint32_t> sourceMappingAllocator;

        // Get the mapping of source in cluster, where this impl corresponds cluster.
        std::pair<CompleteSource, bool> findOrCreateMapping(CompleteSource source, Cluster cluster, utick_t) {
            {
                std::shared_lock g(sourceMappingLock);
                auto it = sourceMappings.find(source);
                if (it != sourceMappings.end())
                    return { it->second, false };
            }
            std::unique_lock g(sourceMappingLock);
            auto id = sourceMappingAllocator.allocateId();
            auto mapped = CompleteSource::mapping(id, cluster);
            sourceMappings.insert({ source, mapped });
            return { mapped, true };
        }

        std::mutex mappedTargetAllocatorLock;
        IdAllocator<uint32_t> mappedTargetAllocator;
        MappedSourceNodeImpl* mappedSourceNodes;

        // Create the target associated to the mapping sourceMapping of source,
        // where this impl corresponds to source.cluster()
        CompleteTarget createMappedTarget(CompleteSource source, CompleteSource sourceMapping, utick_t) {
            std::unique_lock g(mappedTargetAllocatorLock);
            auto id = mappedTargetAllocator.allocateId();
            mappedTargetNodes[id].source = sourceMapping;
            return CompleteTarget::mapped(id, source);
        }

        MappedSourceNodeImpl& findMappedSource(CompleteTarget target, utick_t) {
            return mappedSourceNodes[target.id];
        }

        // All the mapped targets in other cluster that are associated with
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
    };
    ClusterImpl& clusterImpl(Cluster cluster);

    /* ==============> TODO <============== */
    AtomicArrayBuilder<SourceUpdate>& updateQueue(CompleteSource);
    AtomicArrayBuilder<TargetUpdate>& updateQueue(CompleteTarget);
    std::atomic<urefcnt_t>& signal(CompleteTarget, utick_t);

    CompleteSource mapSource(Cluster, CompleteSource, utick_t);
    MappedSourceNodeImpl& findMappedSource(CompleteTarget, utick_t);

    TickExpander tickExpander(Cluster);
    // Translating using this method is only valid if .
    utick_t translateTick(Cluster from, Cluster to, utick_t);
    // Translating to higher harmonics using this method is only valid when
    // the corresponding low bits of tick are 0.
    utick_t translateTickCeil(Cluster from, Cluster to, utick_t);
    utick_t translateTickFloor(Cluster from, Cluster to, utick_t);
    utick_t ceilTickToHarmonic(uint8_t harmonic, utick_t);
    utick_t floorTickToHarmonic(uint8_t harmonic, utick_t);

    void scheduleTask(Task, utick_t);

    SourceNodeImpl& getSourceNode(CompleteSource, utick_t);
    TargetNodeImpl& getTargetNode(CompleteTarget, utick_t);
    int_t getNumAssociatedSources(CompleteTarget, utick_t);
    CompleteSource getAssociatedSource(CompleteTarget, int_t, utick_t);

    /* ==============> Concurrency <============== */
    static constexpr urefcnt_t TARGET_UNLOCKED_FLAG = (urefcnt_t)1 << (std::numeric_limits<urefcnt_t>::digits - 1);

    void signalTarget(CompleteTarget, utick_t, urefcnt_t, Metadata metadata);
    void unlockTarget(TargetNodeImpl&, CompleteTarget, utick_t);
    void signalSource(SourceNodeImpl&, CompleteSource, utick_t);
    void updateDependencyWithinCluster(CompleteSource, CompleteTarget, utick_t sourceTick, tickdiff_t ticksDelay, refcnt_t delta);

    /* ==============> ? <============== */
    CompleteSource updateDependency(CompleteSource source, CompleteTarget target, utick_t sourceTick, tickdiff_t targetTicksDelay, refcnt_t delta);
    void onDepedenciesFulfilled(CompleteTarget, utick_t);
    Target updateDependency(Task source, Task target, utick_t sourceTick, tickdiff_t sourceTicksDelay, refcnt_t delta);
    void beginClusterTick(Cluster cluster, utick_t tick, const std::bitset<MAX_GROUPS>& firstTimeUsingTick);
};

}
