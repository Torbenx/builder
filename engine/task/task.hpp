#pragma once

#include "task/heap.hpp"
#include <coroutine>
#include <new>
#include <type_traits>

namespace Tag {
struct Task {
    static constexpr char tag[] = "task";
    static constexpr bool verifyEnabled = true;
};
}

namespace task {

using urefcnt_t = uint16_t;
using refcnt_t = std::make_signed_t<urefcnt_t>;
using tickdiff_t = int16_t;

constexpr size_t HARMONIC_BITS = 3;
constexpr int_t NUM_HARMONICS = (int_t)1 << HARMONIC_BITS;
constexpr size_t CLUSTER_BITS = 8 - HARMONIC_BITS;

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

struct PromiseImpl;

struct Promise {
    static void* operator new(size_t size, std::align_val_t alignment);
    static void* operator new(size_t size) { return Promise::operator new (size, std::align_val_t { __STDCPP_DEFAULT_NEW_ALIGNMENT__ }); }
    static void operator delete(void* ptr, size_t size, std::align_val_t alignment);
    static void operator delete(void* ptr, size_t size) { return Promise::operator delete (ptr, size, std::align_val_t { __STDCPP_DEFAULT_NEW_ALIGNMENT__ }); }

    struct ReturnObject {
        using promise_type = Promise;
        Promise* promise;
    };

    Promise();

    PromiseImpl& impl;
    std::suspend_always initial_suspend() { return {}; }
    std::suspend_never final_suspend() noexcept { return {}; }
    ReturnObject get_return_object() { return { this }; }
    void return_void() { }
    void unhandled_exception() {
        VERIFY_NOT_REACHED(Task);
        std::abort();
    }
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

struct InterfaceCallInfo {
    CompleteSource caller
    utick_t tick;
};

template<typename T>
concept Interface = requires(T::Storage storage, InterfaceCallInfo info) {
    T(storage, info);
};

template<typename I>
struct InterfaceRef {
    CompleteTarget target;
    uint32_t addr : 24 = 0;
    uint32_t delay : 8 = 0;
};

template<typename D>
struct DataRef {
    CompleteSource source;
    uint32_t addr : 24 = 0;
    uint32_t delay : 8 = 0;
};

struct TaskSelfObject {
    Task task;
    utick_t tick;
    template<typename I>
    I& operator[](InterfaceRef<I> ref) {
        StubAllocator localAlloc { task.cluster(), tick };
        utick_t remoteTick = ceilTickToHarmonic(ref.source.harmonic, localAlloc.dataHeapHeader().ticks.tick(ref.source.cluster()) + ref.delay);
        StubAllocator remoteAlloc { ref.source.cluster(), remoteTick };
        return *(I*)remoteAlloc.toAddress(ref.addr);
    }
    template<typename D>
    D& operator[](DataRef<D> ref) {
        StubAllocator localAlloc { task.cluster(), tick - ref.delay };
        utick_t remoteTick = floorTickToHarmonic(ref.source.harmonic, localAlloc.dataHeapHeader().ticks.tick(ref.source.cluster()));
        StubAllocator remoteAlloc { ref.source.cluster(), remoteTick };
        return *(I*)remoteAlloc.toAddress(ref.addr);
    }
};

}

using Task = task::Promise::ReturnObject;
