#pragma once

#include "containers/ArrayView.hpp"
#include "containers/Tuple.hpp"
#include "util/log.hpp"
#include <atomic>
#include <vector>

namespace Tag {
struct render3d {
    static constexpr char tag[] = "render3d";
    static constexpr bool verifyEnabled = true;
};
}

namespace render3d {

struct SG;
struct RT;

#define IMPORT_NODES(S)                                        \
    using PortalNode = typename S::PortalNode;                 \
    using TransformNode = typename S::TransformNode;           \
    using WorldTransformNode = typename S::WorldTransformNode; \
    using BoundingNode = typename S::BoundingNode;             \
    using ObjectNode = typename S::ObjectNode;                 \
    using CompoundNode = typename S::CompoundNode;

enum class NodeKind {
    Invalid = 0,
    Portal,
    Transform,
    WorldTransform,
    Bounding,
    Object,
    Compound,
};

struct NodeKindSequence {
    static constexpr int_t KIND_BIT_SIZE = 4;
    using mask_t = uint32_t;
    static constexpr mask_t FIRST_NODE_MASK = ((mask_t)1 << KIND_BIT_SIZE) - 1;
    static constexpr int_t MASK_DIGITS = std::numeric_limits<mask_t>::digits;
    static constexpr int_t CAPACITY = MASK_DIGITS / KIND_BIT_SIZE;

    struct iterator;
    struct end_iterator { };

    mask_t sequence = 0;

    constexpr NodeKindSequence() { }
    template<int_t N>
    constexpr NodeKindSequence(const NodeKind (&kinds)[N]) {
        static_assert(N <= CAPACITY);
        for (int_t i = N - 1; i >= 0; i--)
            pushFront(kinds[i]);
    }
    constexpr NodeKindSequence(mask_t sequence)
        : sequence(sequence) { }
    constexpr int_t size() const {
        return (MASK_DIGITS - std::countl_zero(sequence) + KIND_BIT_SIZE - 1) / KIND_BIT_SIZE;
    }
    constexpr bool empty() const { return sequence == 0; }
    constexpr void pushBack(NodeKind kind) {
        sequence |= (mask_t)kind << (size() * KIND_BIT_SIZE);
    }
    constexpr void pushFront(NodeKind kind) {
        sequence <<= KIND_BIT_SIZE;
        sequence |= (mask_t)kind;
    }
    constexpr NodeKind operator[](int_t index) const {
        return static_cast<NodeKind>((sequence >> (index * KIND_BIT_SIZE)) & FIRST_NODE_MASK);
    }
    constexpr NodeKind front() const { return static_cast<NodeKind>(sequence & FIRST_NODE_MASK); }
    constexpr NodeKind back() const { return (*this)[size() - 1]; }
    constexpr NodeKind popFront() {
        NodeKind r = front();
        sequence >>= KIND_BIT_SIZE;
        return r;
    }
    constexpr NodeKindSequence subSequence() const {
        return { sequence >> KIND_BIT_SIZE };
    }
    constexpr NodeKind popBack() {
        int_t newSize = size() - 1;
        NodeKind r = (*this)[newSize];
        sequence &= ((mask_t)1 << (newSize * KIND_BIT_SIZE)) - 1;
        return r;
    }
    constexpr iterator begin() const;
    constexpr end_iterator end() const { return {}; }
};
struct NodeKindSequence::iterator : NodeKindSequence {
    constexpr iterator& operator++() {
        popFront();
        return *this;
    }
    constexpr NodeKind operator*() const { return front(); }
    constexpr bool operator==(const end_iterator&) const { return empty(); }
};
constexpr NodeKindSequence::iterator NodeKindSequence::begin() const { return { *this }; }

struct NodeHandle {
    NodeKind kind = NodeKind::Invalid;
    void* node = nullptr;
    constexpr NodeHandle() { }
    constexpr NodeHandle(NodeKind kind, void* node)
        : kind(kind), node(node) { }
    template<typename T>
    constexpr NodeHandle(T* node)
        : kind(T::NODE_KIND), node(node) { }
};
template<typename S>
struct NodeSequence {
    static constexpr int_t sizeOf(NodeKind kind);
    static constexpr int_t sequenceSizeInBytes(NodeKindSequence kinds) {
        int_t size = 0;
        for (auto kind : kinds)
            size += sizeOf(kind);
        return size;
    }
    static constexpr NodeSequence allocateStorageFor(NodeKindSequence kinds) {
        int_t size = sequenceSizeInBytes(kinds);
        std::allocator<std::byte> allocator;
        std::byte* storage = allocator.allocate(size);
        return { storage };
    }
    static constexpr NodeSequence adoptKinds(void* begin, NodeKindSequence kinds) {
        return { static_cast<std::byte*>(begin), kinds, static_cast<uint32_t>(sequenceSizeInBytes(kinds)) };
    }
    template<typename T>
    static constexpr int_t sizeOf() {
        if (std::is_empty_v<T>)
            return 0;
        return sizeof(T);
    }
    template<typename T>
    static constexpr bool isSizeOfValid() {
        return sizeOf<T>() == sizeOf(T::NODE_KIND);
    }

    std::byte* begin = nullptr;
    NodeKindSequence kinds {};
    uint32_t sizeInBytes = 0;

    bool empty() const { return kinds.empty(); }
    int_t size() const { return kinds.size(); }

    NodeKind frontKind() const { return kinds.front(); }
    NodeHandle front() const { return { kinds.front(), begin }; }
    NodeHandle popFront() {
        NodeKind kind = kinds.popFront();
        int_t advance = sizeOf(kind);
        void* element = begin;
        begin += advance;
        sizeInBytes -= advance;
        return { kind, element };
    }
    template<typename T>
    T* frontAs() const { return reinterpret_cast<T*>(begin); }
    NodeSequence subSequence() const {
        NodeKind kind = kinds.front();
        int_t advance = sizeOf(kind);
        return { begin + advance, kinds.subSequence(), static_cast<uint32_t>(sizeInBytes - advance) };
    }

    NodeKind backKind() const { return kinds.back(); }
    NodeHandle back() const {
        NodeKind kind = kinds.back();
        return { kind, begin + sizeInBytes - sizeOf(kind) };
    }
    NodeHandle popBack() {
        NodeKind kind = kinds.popBack();
        sizeInBytes -= sizeOf(kind);
        return { kind, begin + sizeInBytes };
    }
    template<typename T>
    T* pushBack(const T& node) {
        T* r = std::construct_at(reinterpret_cast<T*>(begin + sizeInBytes), node);
        sizeInBytes += sizeOf<T>();
        kinds.pushBack(T::NODE_KIND);
        return r;
    }
    NodeHandle pushBackEmpty(NodeKind kind) {
        sizeInBytes += sizeOf(kind);
        kinds.pushBack(kind);
        return back();
    }
    NodeHandle endAs(NodeKind kind) {
        return { kind, begin + sizeInBytes };
    }

    template<typename T>
    struct FirstIs;

    template<typename T>
    FirstIs<T> firstIs() const;

    ~NodeSequence() {
        IMPORT_NODES(S)
        static_assert(isSizeOfValid<PortalNode>());
        static_assert(isSizeOfValid<TransformNode>());
        static_assert(isSizeOfValid<WorldTransformNode>());
        static_assert(isSizeOfValid<BoundingNode>());
        static_assert(isSizeOfValid<ObjectNode>());
        static_assert(isSizeOfValid<CompoundNode>());
    }
};

template<typename S>
template<typename T>
struct NodeSequence<S>::FirstIs : NodeSequence<S> {
    T* first() const { return reinterpret_cast<T*>(begin); }
};

template<typename S>
template<typename T>
typename NodeSequence<S>::template FirstIs<T> NodeSequence<S>::firstIs() const { return { *this }; }

template<typename S>
constexpr int_t NodeSequence<S>::sizeOf(NodeKind kind) {
    IMPORT_NODES(S)
    // must match the NodeKind enum
    constexpr int_t byteSizes[] = { 0, sizeOf<PortalNode>(), sizeOf<TransformNode>(), sizeOf<WorldTransformNode>(), sizeOf<BoundingNode>(), sizeOf<ObjectNode>(), sizeOf<CompoundNode>() };
    return byteSizes[(int_t)kind];
}

template<typename... NodeTypes>
struct StaticNodeSequence : stx::Tuple<NodeTypes...> {
    static constexpr NodeKindSequence kinds { { NodeTypes::NODE_KIND... } };
    operator NodeSequence<SG>() {
        return { (std::byte*)this, kinds, (uint32_t)sizeof(StaticNodeSequence) };
    }
};

}
