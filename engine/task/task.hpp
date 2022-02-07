#pragma once

namespace task {

using utick_t = uint32_t;
using urefcnt_t = uint16_t;
using refcnt_t = std::make_signed_t<urefcnt_t>;
using tickdiff_t = int16_t;

constexpr size_t HARMONICS_BITS = 3;
constexpr size_t NUM_HARMONICS = (size_t)1 << HARMONICS_BITS;
constexpr size_t CLUSTER_BITS = 8 - HARMONICS_BITS;

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

struct Address {
    uint64_t addr : 24;
    uint64_t cluster : 5;
    uint64_t padding : 3 = 0;
    uint64_t tick : 24;
    uint64_t base : 8 = 0x3c;

    Address& withAddr(uint32_t addr) {
        this->addr = addr;
        return *this;
    }
    Address& withCluster(Cluster cluster) {
        this->cluster = cluster;
        return *this;
    }
    Address& withTick(utick_t tick) {
        this->tick = tick;
        return *this;
    }
    Address& addToAddr(uint32_t add) {
        addr += add;
    }
    uint64_t uint() {
        return std::bit_cast<uint64_t>(*this);
    }
    void* ptr() {
        return (void*)uint();
    }
};

static constexpr std::uintptr_t HEAP_BASE_ADDRESS = 0x3c00'0000'0000'0000;

// heap layout
constexpr int_t ADDRESS_BITS = 24;
constexpr uint32_t TEMPORARY_INTERFACE_HEAP_BEGIN = 0;
constexpr uint32_t PERSISTENT_INTERFACE_HEAP_BEGIN = (uint32_t)1 << 16;
constexpr uint32_t PERSISTENT_DATA_HEAP_BEGIN = (uint32_t)1 << (ADDRESS_BITS - 2);
constexpr uint32_t TEMPORARY_DATA_HEAP_BEGIN = (uint32_t)1 << (ADDRESS_BITS - 1);

constexpr uint32_t DATA_HEAP_BEGIN = PERSISTENT_DATA_HEAP_BEGIN;
constexpr uint32_t INTERFACE_HEAP_BEGIN = TEMPORARY_INTERFACE_HEAP_BEGIN;
constexpr int_t ADDRESS_SPACE_END = (int_t)1 << ADDRESS_BITS;
constexpr int_t TEMPORARY_DATA_HEAP_SIZE = ADDRESS_SPACE_END - TEMPORARY_DATA_HEAP_BEGIN;
constexpr int_t PERSISTENT_DATA_HEAP_SIZE = TEMPORARY_DATA_HEAP_BEGIN - PERSISTENT_DATA_HEAP_BEGIN;
constexpr int_t PERSISTENT_INTERFACE_HEAP_SIZE = PERSISTENT_DATA_HEAP_BEGIN - PERSISTENT_INTERFACE_HEAP_BEGIN;
constexpr int_t DATA_HEAP_SIZE = ADDRESS_SPACE_END - DATA_HEAP_BEGIN;
constexpr int_t INTERFACE_HEAP_SIZE = DATA_HEAP_BEGIN - INTERFACE_HEAP_BEGIN;

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

struct HeapAllocation {
    uint32_t offset = 0;
    uint32_t size = 0;
};

struct GraphUpdate;

struct DataHeapBumpAllocatorStub : AllocatorBase<DataHeapBumpAllocatorStub> {
    using uintptr_t = uint32_t;
    template<typename T = void>
    using Ptr = Ptr<BasicRawPtr<uintptr_t>, T>;

    void* bumpBeginAddress();
    uintptr_t internalBumpBeginAddress() {
        return TEMPORARY_DATA_HEAP_BEGIN;
    }
};

struct InterfaceHeapBumpAllocatorStub {
    using uintptr_t = uint16_t;
    template<typename T = void>
    using Ptr = Ptr<BasicRawPtr<uintptr_t>, T>;

    void* bumpBeginAddress();
    uintptr_t internalBumpBeginAddress() {
        return TEMPORARY_INTERFACE_HEAP_BEGIN;
    }
};

struct DataHeapHeader {
    ExternalTicks ticks;
    AtomicBumpAllocator<>::Data bumpAllocator;
    HalfAtomicArrayBuilder<HeapAllocation> freedAllocations;
    HalfAtomicArrayBuilder<GraphUpdate> graphUpdates;
    std::atomic<int8_t> refCount = 0;
};

struct InterfaceHeapHeader {
    std::atomic<uint16_t> bumpAllocatorOffset = 0;
    std::atomic<uint16_t> bumpAllocatorPageOffset = 0;
    HalfAtomicArrayBuilder<HeapAllocation> freedAllocations;
};

struct ClusterInitializaionBounds {
    int_t maxTasks;
    int_t maxMappedTargets;
    int_t maxSourceMappings;
    int_t maxInterfaceTicksDelay;
    int_t maxInterfaceHeapSize;
    int_t maxDataHeapSize;

    int_t totalInfaceHeapSize() {
        return sizeof(InterfaceHeapHeader)
            + (maxTasks + maxMappedTargets) * sizeof(urefcnt_t)
            + maxInterfaceHeapSize;
    }

    int_t totalDataHeapSize() {
        return sizeof(DataHeapHeader)
            + maxDataHeapSize;
    }
};

}
