#pragma once

#include "containers/AtomicArrayBuilder.hpp"
#include "containers/AtomicBumpAllocator.hpp"
#include "containers/HalfAtomicArrayBuilder.hpp"
#include "memory/Allocator.hpp"
#include "memory/Ptr.hpp"
#include "util/types.hpp"
#include <bitset>

namespace task {

using utick_t = uint32_t;

struct Cluster {
    uint8_t id = 0;

    bool operator==(const Cluster&) const = default;
};

struct Address {
    uint64_t addr : 24;
    uint64_t cluster : 5;
    uint64_t padding : 3 = 0;
    uint64_t tick : 8;
    uint64_t base : 24 = 0x3c;
    static constexpr size_t ADDRESS_SPACE_SIZE = (size_t)1 << 40;

    Address withAddr(uint32_t addr) {
        this->addr = addr;
        return *this;
    }
    Address withCluster(Cluster cluster) {
        this->cluster = cluster.id;
        return *this;
    }
    Address withTick(utick_t tick) {
        this->tick = tick;
        return *this;
    }
    Address addToAddr(uint32_t add) {
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

struct DataHeapHeader;
struct InterfaceHeapHeader;

struct StubAllocator : AllocatorUtil<StubAllocator, uint32_t> {
    static constexpr Ptr<DataHeapHeader> DATA_HEAP_HEADER = { PERSISTENT_DATA_HEAP_BEGIN };
    static constexpr Ptr<InterfaceHeapHeader> INTERFACE_HEAP_HEADER = { PERSISTENT_INTERFACE_HEAP_BEGIN };

    using Parameter = StubAllocator;

    Cluster cluster;
    utick_t tick;

    StubAllocator(Cluster cluster, utick_t tick)
        : cluster(cluster), tick(tick) { }
    StubAllocator(Object, StubAllocator s)
        : StubAllocator(s) { }
    Address tickBase() {
        return Address().withTick(tick).withCluster(cluster);
    }
    void* toAddress(uintptr_t addr) {
        return tickBase().withAddr(addr).ptr();
    }
    DataHeapHeader& dataHeapHeader() {
        return *(*this)[DATA_HEAP_HEADER];
    }
    InterfaceHeapHeader& interfaceHeapHeader() {
        return *(*this)[INTERFACE_HEAP_HEADER];
    }
};

/* ==============> Temporary allocators <============== */

struct TemporaryDataStubAllocator : StubAllocator {
    using StubAllocator::StubAllocator;
    void* bumpBeginAddress() {
        return tickBase().withAddr(TEMPORARY_DATA_HEAP_BEGIN).ptr();
    }
    uintptr_t internalBumpBeginAddress() {
        return TEMPORARY_DATA_HEAP_BEGIN;
    }
};

struct TemporaryInterfaceStubAllocator : StubAllocator {
    using StubAllocator::StubAllocator;
    void* bumpBeginAddress() {
        return tickBase().withAddr(TEMPORARY_INTERFACE_HEAP_BEGIN).ptr();
    }
    uintptr_t internalBumpBeginAddress() {
        return TEMPORARY_INTERFACE_HEAP_BEGIN;
    }
};

using TemporaryDataBlockAllocator = typename PageBlockAllocator<TemporaryDataStubAllocator>::ParameterRef;
using TemporaryInterfaceBlockAllocator = typename PageBlockAllocator<TemporaryInterfaceStubAllocator>::ParameterRef;

template<typename BlockAllocator>
using TemporaryAllocator = typename ContinuousAtomicBumpAllocator<BlockAllocator, 16, BlockAllocator::BLOCK_SIZE>::ParameterRef;
using TemporaryDataAllocator = TemporaryAllocator<TemporaryDataBlockAllocator>;
using TemporaryInterfaceAllocator = TemporaryAllocator<TemporaryInterfaceBlockAllocator>;

/* ==============> Persistent allocators <============== */

struct HeapAllocation {
    StubAllocator::Ptr<> ptr;
    uint32_t size = 0;
};

struct PersistentAllocator : StubAllocator {
    using Parameter = PersistentAllocator;

    HalfAtomicArrayBuilder<HeapAllocation, TemporaryInterfaceAllocator>& freeQueue;

    PersistentAllocator(StubAllocator s, HalfAtomicArrayBuilder<HeapAllocation, TemporaryInterfaceAllocator>& freeQueue)
        : StubAllocator(s), freeQueue(freeQueue) { }
    PersistentAllocator(Object, PersistentAllocator p)
        : PersistentAllocator(p) { }

    void freeImpl(size_t alignment, Ptr<> ptr, int_t size);
};

struct PersistentDataAllocator : PersistentAllocator, AllocatorMethods<PersistentDataAllocator, uint32_t> {
    using PersistentAllocator::PersistentAllocator;
    Ptr<> allocateImpl(size_t alignment, int_t size) {
        return allocateInternal(*this, alignment, size);
    }

private:
    static Ptr<> allocateInternal(StubAllocator, size_t alignment, int_t size);
};

struct PersistentInterfaceAllocator : PersistentAllocator, AllocatorMethods<PersistentInterfaceAllocator, uint32_t> {
    using PersistentAllocator::PersistentAllocator;
    Ptr<> allocateImpl(size_t alignment, int_t size) {
        return allocateInternal(*this, alignment, size);
    }

private:
    static Ptr<> allocateInternal(StubAllocator, size_t alignment, int_t size);
};

/* ==============> Heap headers <============== */

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

struct DataHeapHeader {
    ExternalTicks ticks;
    std::atomic<int8_t> refCount = 0;

    DataHeapHeader();
};

struct GraphUpdate;

struct InterfaceHeapHeader {
    HalfAtomicArrayBuilder<GraphUpdate, TemporaryInterfaceAllocator> graphUpdates;

    TemporaryInterfaceBlockAllocator::Data temporaryInterfaceBlockAllocator;
    TemporaryInterfaceAllocator::Data temporaryInterfaceAllocator;
    HalfAtomicArrayBuilder<HeapAllocation, TemporaryInterfaceAllocator> freedInterfaceAllocations;

    TemporaryDataBlockAllocator::Data temporaryDataBlockAllocator;
    TemporaryDataAllocator::Data temporaryDataAllocator;
    HalfAtomicArrayBuilder<HeapAllocation, TemporaryInterfaceAllocator> freedDataAllocations;

    TemporaryDataAllocator::Parameter temporaryDataParameter(StubAllocator stub) {
        return { temporaryDataAllocator, { temporaryDataBlockAllocator, stub } };
    }
    TemporaryInterfaceAllocator::Parameter temporaryInterfaceParameter(StubAllocator stub) {
        return { temporaryInterfaceAllocator, { temporaryInterfaceBlockAllocator, stub } };
    }
    PersistentDataAllocator persistentDataParameter(StubAllocator stub) {
        return { stub, freedDataAllocations };
    }
    PersistentInterfaceAllocator persistentInterfaceParameter(StubAllocator stub) {
        return { stub, freedInterfaceAllocations };
    }

    InterfaceHeapHeader();
};

/* ==============> Persistent allocators <============== */

inline void PersistentAllocator::freeImpl([[maybe_unused]] size_t alignment, Ptr<> ptr, int_t size) {
    freeQueue.pushBack({ ptr.addr(), (uint32_t)size }, interfaceHeapHeader().temporaryInterfaceParameter(*this));
}

}
