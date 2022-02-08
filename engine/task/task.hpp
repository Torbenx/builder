#pragma once

#include "containers/AtomicArrayBuilder.hpp"
#include "containers/AtomicBumpAllocator.hpp"
#include "containers/HalfAtomicArrayBuilder.hpp"
#include "memory/Allocator.hpp"
#include "memory/Ptr.hpp"
#include "util/types.hpp"
#include <atomic>
#include <bit>
#include <bitset>
#include <type_traits>

namespace Tag {
struct Task {
    static constexpr char tag[] = "task";
    static constexpr bool verifyEnabled = true;
};
}

namespace task {

using utick_t = uint32_t;
using urefcnt_t = uint16_t;
using refcnt_t = std::make_signed_t<urefcnt_t>;
using tickdiff_t = int16_t;

constexpr size_t HARMONIC_BITS = 3;
constexpr int_t NUM_HARMONICS = (int_t)1 << HARMONIC_BITS;
constexpr size_t CLUSTER_BITS = 8 - HARMONIC_BITS;

struct Cluster {
    uint8_t id = 0;

    bool operator==(const Cluster&) const = default;
};

struct Task;

struct CompleteNode {
    uint16_t id = 0;
    uint8_t clusterId : CLUSTER_BITS = 0;
    uint8_t harmonic : HARMONIC_BITS = 0;
    bool mapping = false;

    Cluster cluster() { return { clusterId }; }
    bool isMapping() { return mapping; }
    Task asTask();

    bool operator==(const CompleteNode&) const = default;
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

    bool operator==(const CompleteSource&) const = default;
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
    uint8_t harmonic : HARMONIC_BITS = 0;

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

    bool operator==(const Task&) const = default;
};

inline Task CompleteNode::asTask() {
    return {
        .id = id,
        .clusterId = clusterId,
        .harmonic = harmonic,
    };
}

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
        this->cluster = cluster.id;
        return *this;
    }
    Address& withTick(utick_t tick) {
        this->tick = tick;
        return *this;
    }
    Address& addToAddr(uint32_t add) {
        addr += add;
        return *this;
    }
    uint64_t uint() {
        return std::bit_cast<uint64_t>(*this);
    }
    void* ptr() {
        return (void*)uint();
    }
};

struct TickExpander {
    uint64_t high = 0;
    int_t tick(uint32_t utick) {
        return (utick ^ (uint32_t)high) + high;
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
constexpr uint32_t ADDRESS_SPACE_END = (int_t)1 << ADDRESS_BITS;
constexpr int_t TEMPORARY_DATA_HEAP_SIZE = ADDRESS_SPACE_END - TEMPORARY_DATA_HEAP_BEGIN;
constexpr int_t PERSISTENT_DATA_HEAP_SIZE = TEMPORARY_DATA_HEAP_BEGIN - PERSISTENT_DATA_HEAP_BEGIN;
constexpr int_t PERSISTENT_INTERFACE_HEAP_SIZE = PERSISTENT_DATA_HEAP_BEGIN - PERSISTENT_INTERFACE_HEAP_BEGIN;
constexpr int_t DATA_HEAP_SIZE = ADDRESS_SPACE_END - DATA_HEAP_BEGIN;
constexpr int_t INTERFACE_HEAP_SIZE = DATA_HEAP_BEGIN - INTERFACE_HEAP_BEGIN;

struct ExternalTicks {
    std::bitset<32> dependenceMask = 0;
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

struct DataHeapBumpAllocatorStub : AllocatorBase<DataHeapBumpAllocatorStub, uint32_t> {
    void* bumpBeginAddress();
    uintptr_t internalBumpBeginAddress() {
        return TEMPORARY_DATA_HEAP_BEGIN;
    }
};

struct InterfaceHeapBumpAllocatorStub : AllocatorBase<InterfaceHeapBumpAllocatorStub, uint16_t> {
    struct Parameter {
        Cluster cluster;
        utick_t tick;
    };
    Parameter param;
    InterfaceHeapBumpAllocatorStub(Object, Parameter p)
        : param(p) { }
    void* bumpBeginAddress() {
        return Address()
            .withCluster(param.cluster)
            .withTick(param.tick)
            .withAddr(TEMPORARY_INTERFACE_HEAP_BEGIN)
            .ptr();
    }
    uintptr_t internalBumpBeginAddress() {
        return TEMPORARY_INTERFACE_HEAP_BEGIN;
    }

    template<typename T>
    T* operator[](Ptr<T> ptr) {
        return (T*)Address()
            .withCluster(param.cluster)
            .withTick(param.tick)
            .withAddr(ptr.addr())
            .ptr();
    }
};

using DataHeapBumpBlockAllocator = typename PageBlockAllocator<DataHeapBumpAllocatorStub>::ParameterRef;
using InterfaceHeapBumpBlockAllocator = typename PageBlockAllocator<InterfaceHeapBumpAllocatorStub>::ParameterRef;

template<typename BlockAllocator>
using BumpAllocator = typename ContinuousAtomicBumpAllocator<BlockAllocator, 16, BlockAllocator::BLOCK_SIZE>::ParameterRef;
using DataHeapBumpAllocator = BumpAllocator<DataHeapBumpBlockAllocator>;
using InterfaceHeapBumpAllocator = BumpAllocator<InterfaceHeapBumpBlockAllocator>;

struct DataHeapHeader {
    ExternalTicks ticks;
    DataHeapBumpBlockAllocator::Data bumpBlockAllocator;
    DataHeapBumpAllocator::Data bumpAllocator;
    HalfAtomicArrayBuilder<HeapAllocation, InterfaceHeapBumpAllocator> freedAllocations;
    HalfAtomicArrayBuilder<GraphUpdate, InterfaceHeapBumpAllocator> graphUpdates;
    std::atomic<int8_t> refCount = 0;

    DataHeapHeader();
};

struct InterfaceHeapHeader {
    InterfaceHeapBumpBlockAllocator::Data bumpBlockAllocator;
    InterfaceHeapBumpAllocator::Data bumpAllocator;
    HalfAtomicArrayBuilder<HeapAllocation, InterfaceHeapBumpAllocator> freedAllocations;

    InterfaceHeapBumpAllocator::Parameter bumpParameter(Cluster cluster, utick_t tick) {
        return { bumpAllocator, { bumpBlockAllocator, { cluster, tick } } };
    }

    InterfaceHeapHeader();
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
        return sizeof(DataHeapHeader) + maxDataHeapSize;
    }
};

struct Promise { };

}
