#pragma once

#include "task/heap.hpp"
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

}
