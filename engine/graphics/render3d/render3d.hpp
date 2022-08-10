#pragma once

#include "containers/ArrayView.hpp"
#include "containers/BitSet.hpp"
#include "containers/Tuple.hpp"
#include "util/log.hpp"
#include <atomic>
#include <memory>
#include <vector>

namespace Tag {
struct render3d {
    static constexpr char tag[] = "render3d";
    static constexpr bool verifyEnabled = true;
};
}

namespace render3d {

struct BoundingSphere {
    Vec3 position;
    float radius;
};

struct CullStack {
    struct Plane {
        Vec3 unitNormal;
        float w;
        Plane transform(const Mat4x4& transform) const {
            Vec4 v { unitNormal, 0.0f };
            v = transform * v;
            Vec3 translation = { transform.matrix[12], transform.matrix[13], transform.matrix[14] };
            float newW = w + Vec3::dot(translation, v.xyz);
            return { v.xyz, newW };
        }
    };
    std::vector<Plane> planes;
    bool visible(BoundingSphere bs) const {
        for (auto plane : planes) {
            if (Vec3::dot(bs.position, plane.unitNormal) + plane.w >= bs.radius)
                return false;
        }
        return true;
    }
    void set(const CullStack* baseStack, const Mat4x4& transform, const CullStack& append) {
        clear();
        if (baseStack)
            planes = baseStack->planes;
        for (auto plane : append.planes)
            planes.push_back(plane.transform(transform));
    }
    void clear() { planes.clear(); }
};

#define IMPORT_NODES(S)                              \
    using PortalNode = typename S::PortalNode;       \
    using TransformNode = typename S::TransformNode; \
    using BoundingNode = typename S::BoundingNode;   \
    using ObjectNode = typename S::ObjectNode;       \
    using CompoundNode = typename S::CompoundNode;   \
    using WorldCoordinatesMarkerNode = typename S::WorldCoordinatesMarkerNode;

enum class NodeKind {
    Invalid = 0,
    Portal,
    Transform,
    Bounding,
    Object,
    Compound,
    WorldCoordinatesMarker,
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
    NodeKindSequence kinds;
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
    struct FirstIs : NodeSequence {
        T* first() const { return reinterpret_cast<T*>(begin); }
    };
    template<typename T>
    struct LastIs : NodeSequence {
        T* last() const { return reinterpret_cast<T*>(begin + sizeInBytes - sizeOf<T>()); }
    };

    template<typename T>
    FirstIs<T> firstIs() const { return { *this }; }
    template<typename T>
    LastIs<T> lastIs() const { return { *this }; }

    ~NodeSequence() {
        IMPORT_NODES(S)
        static_assert(isSizeOfValid<PortalNode>());
        static_assert(isSizeOfValid<TransformNode>());
        static_assert(isSizeOfValid<BoundingNode>());
        static_assert(isSizeOfValid<ObjectNode>());
        static_assert(isSizeOfValid<CompoundNode>());
        static_assert(isSizeOfValid<WorldCoordinatesMarkerNode>());
    }
};

template<typename S>
constexpr int_t NodeSequence<S>::sizeOf(NodeKind kind) {
    IMPORT_NODES(S)
    // must match the NodeKind enum
    constexpr int_t byteSizes[] = { 0, sizeOf<PortalNode>(), sizeOf<TransformNode>(), sizeOf<BoundingNode>(), sizeOf<ObjectNode>(), sizeOf<CompoundNode>(), sizeOf<WorldCoordinatesMarkerNode>() };
    return byteSizes[(int_t)kind];
}

struct SG /* Scene Graph */ {
    struct alignas(8) PortalNode {
        static constexpr NodeKind NODE_KIND = NodeKind::Portal;
        bool b = false;
        CullStack planes;
    };
    struct alignas(8) TransformNode {
        static constexpr NodeKind NODE_KIND = NodeKind::Transform;
        Mat4x4 transform;
    };
    struct alignas(8) BoundingNode {
        static constexpr NodeKind NODE_KIND = NodeKind::Bounding;
        BoundingSphere bound;
    };
    struct alignas(8) ObjectNode {
        static constexpr NodeKind NODE_KIND = NodeKind::Object;
        const GraphicsObject* object;
        std::vector<Effect::Handle> effects;
    };
    struct alignas(8) CompoundNode {
        static constexpr NodeKind NODE_KIND = NodeKind::Compound;
        enum class UpdateKind {
            Insertion,
            Update,
            Removal,
        };
        struct Update {
            UpdateKind kind;
            bool isObjectSeq;
            int_t index;
        };
        std::vector<NodeSequence<SG>> m_subSeqs;
        std::vector<Update> updates;

        ArrayView<NodeSequence<SG>> subSeqs() { return m_subSeqs; }
        void clearUpdates() { updates.clear(); }
        void addSubSequence(NodeSequence<SG> seq) {
            // updates.push_back({ UpdateKind::Insertion, true, (int_t)objectSeqs.size() });
            m_subSeqs.push_back(seq);
        }
    };
    struct alignas(8) WorldCoordinatesMarkerNode {
        static constexpr NodeKind NODE_KIND = NodeKind::WorldCoordinatesMarker;
    };
};

namespace detail {
    template<typename... Types>
    struct LastPackParameter;
    template<typename T, typename... Types>
    struct LastPackParameter<T, Types...> : LastPackParameter<Types...> { };
    template<typename T>
    struct LastPackParameter<T> {
        using type = T;
    };
}
template<typename... NodeTypes>
struct StaticNodeSequence : stx::Tuple<NodeTypes...> {
    static constexpr NodeKindSequence kinds { { NodeTypes::NODE_KIND... } };
    operator NodeSequence<SG>::LastIs<typename detail::LastPackParameter<NodeTypes...>::type>() {
        return { (std::byte*)this, kinds, (uint32_t)sizeof(StaticNodeSequence) };
    }
};

struct RT /* Rendering Tree */ {
    struct PortalNode;
    struct TransformNode;
    struct BoundingNode;
    struct ObjectNode;
    struct CompoundNode;

    struct PortalItem {
        RT::TransformNode* worldTransform = nullptr;
        RT::PortalNode* sourceStack = nullptr;
        NodeSequence<SG>::FirstIs<SG::PortalNode> input;
        RT::PortalNode* output;
        bool update() {
            output->stack.set(sourceStack ? &sourceStack->stack : nullptr, worldTransform->transform, input.first()->planes);
            return true;
        }
    };
    struct TransformItem {
        RT::TransformNode* sourceTransform = nullptr;
        SG::TransformNode* input = nullptr;
        RT::TransformNode* output = nullptr;
        void update() {
            output->transform = sourceTransform->transform * input->transform;
        }
    };
    struct BoundingItem {
        RT::TransformNode* worldTransform = nullptr;
        NodeSequence<SG>::FirstIs<SG::BoundingNode> input;
        RT::BoundingNode* output = nullptr;
        bool update(const CullStack& stack) {
            auto bound = input.first()->bound;
            Vec3 pos = (worldTransform->transform * Vec4(bound.position, 1.0)).xyz;
            bool wasVisible = output->visible;
            output->visible = stack.visible({ pos, bound.radius });
            return wasVisible;
        }
    };
    struct ObjectItem {
        RT::TransformNode* worldTransform = nullptr;
        SG::ObjectNode* input = nullptr;
        RT::ObjectNode* output = nullptr;
    };

    enum class NodeState {
        Alive,
        Dead,
    };
    struct alignas(8) Sentinel {
        std::atomic<NodeState> state = NodeState::Alive;
        constexpr Sentinel() { }
        constexpr Sentinel(const Sentinel& other)
            : state(other.state.load(std::memory_order_relaxed)) { }
        constexpr Sentinel(NodeState state)
            : state(state) { }
    };
    struct PortalNode : Sentinel {
        static constexpr NodeKind NODE_KIND = NodeKind::Portal;
        CullStack stack;
        std::vector<BoundingItem> boundingItems;
    };
    struct TransformNode : Sentinel {
        static constexpr NodeKind NODE_KIND = NodeKind::Transform;
        Mat4x4 transform;
    };
    struct BoundingNode : Sentinel {
        static constexpr NodeKind NODE_KIND = NodeKind::Bounding;
        bool visible = false;
    };
    struct ObjectNode : Sentinel {
        static constexpr NodeKind NODE_KIND = NodeKind::Object;
    };
    struct CompoundNode {
        static constexpr NodeKind NODE_KIND = NodeKind::Compound;

        RT::TransformNode* worldTransform;
        RT::PortalNode* portalStack;
        std::atomic<uintptr_t> subSequenceStorage = (uintptr_t)0 | (uintptr_t)1;

        CompoundNode(RT::TransformNode* worldTransform, RT::PortalNode* portalStack)
            : worldTransform(worldTransform), portalStack(portalStack) { }
        CompoundNode(const CompoundNode& other)
            : worldTransform(other.worldTransform), portalStack(other.portalStack), subSequenceStorage(other.subSequenceStorage.load(std::memory_order_relaxed)) { }

        struct ReturnVal {
            ArrayView<NodeSequence<RT>> subSeqs;
            NodeState state;
        };
        struct Allocation {
            int_t* buffer = nullptr;
            int_t size = 0;
            Allocation() { }
            Allocation(const Allocation&) = delete;
            Allocation(Allocation&& other)
                : buffer(other.buffer), size(other.size) {
                other.clearAndLeak();
            }
            Allocation(int_t capacity) {
                buffer = (int_t*)new std::byte[sizeof(int_t) + sizeof(NodeSequence<RT>) * capacity];
            }
            Allocation(int_t* buffer, int_t size)
                : buffer(buffer), size(size) { }
            ~Allocation() { delete[](std::byte*) buffer; }
            void clearAndLeak() {
                buffer = nullptr;
                size = 0;
            }
            void pushBack(NodeSequence<RT> seq) {
                *view().end() = seq;
                size += 1;
            }
            ArrayView<NodeSequence<RT>> view() { return { (NodeSequence<RT>*)(buffer + 1), size }; };
        };
        struct Replacement : Allocation {
            uintptr_t oldStorage = 0;
        };
        ReturnVal toReturnVal(uintptr_t storage) const {
            NodeState state = (storage & (uintptr_t)1) ? NodeState::Alive : NodeState::Dead;
            int_t* buffer = (int_t*)(storage & ~(uintptr_t)1);
            if (!buffer)
                return { {}, state };
            return { { (NodeSequence<RT>*)(buffer + 1), *buffer }, state };
        }
        ReturnVal get() const {
            uintptr_t storage = subSequenceStorage.load(std::memory_order_acquire);
            return toReturnVal(storage);
        }
        ArrayView<NodeSequence<RT>> subSeqs() const { return get().subSeqs; }
        Allocation take() {
            uintptr_t storage = subSequenceStorage.exchange(0, std::memory_order_acquire);
            int_t* buffer = (int_t*)(storage & ~(uintptr_t)1);
            return { buffer, *buffer };
        }
        Replacement beginReplace(int_t extraCapacity) const {
            uintptr_t storage = subSequenceStorage.load(std::memory_order_acquire);
            auto oldView = toReturnVal(storage).subSeqs;
            Replacement rep { { oldView.size() + extraCapacity }, storage };
            rep.size = oldView.size();
            std::copy(oldView.begin(), oldView.end(), rep.view().begin());
            return rep;
        }
        bool endReplace(Replacement rep) {
            *rep.buffer = rep.size;
            uintptr_t newStorage = ((uintptr_t)rep.buffer) | (uintptr_t)1;
            uintptr_t expected = rep.oldStorage;
            bool success = subSequenceStorage.compare_exchange_strong(expected, newStorage, std::memory_order_release);
            VERIFY(render3d, success || expected == 0);
            if (success)
                rep.clearAndLeak();
            return success;
        }
    };
    struct WorldCoordinatesMarkerNode : Sentinel {
        static constexpr NodeKind NODE_KIND = NodeKind::WorldCoordinatesMarker;
    };

    struct CoalescedUpdates {
        RT::TransformNode rootTransform { {}, Mat4x4::Id };
        std::vector<SG::TransformNode*> cameraTransforms;
        std::vector<TransformItem> transformItems;
        std::vector<PortalItem> portalItems;
        std::vector<ObjectItem> objectItems;
    };
    static inline Sentinel* asSentinel(NodeHandle node) {
        return (Sentinel*)node.node;
    }
};

template<typename S, typename Impl>
struct Visitor {
    IMPORT_NODES(S);
    Impl* impl() { return static_cast<Impl*>(this); }
    void visitSequence(NodeSequence<S> seq) {
        impl()->dispatchVisit(seq);
    }
    void dispatchVisit(NodeSequence<S> seq) {
        switch (seq.frontKind()) {
        case NodeKind::Portal:
            impl()->visitPortalNode(seq.template firstIs<PortalNode>());
            break;
        case NodeKind::Transform:
            impl()->visitTransformNode(seq.template firstIs<TransformNode>());
            break;
        case NodeKind::Bounding:
            impl()->visitBoundingNode(seq.template firstIs<BoundingNode>());
            break;
        case NodeKind::Object:
            impl()->visitObjectNode(seq.template firstIs<ObjectNode>());
            break;
        case NodeKind::Compound:
            impl()->visitCompoundNode(seq.template firstIs<CompoundNode>());
            break;
        case NodeKind::WorldCoordinatesMarker:
            impl()->visitWorldCoordinatesMarkerNode(seq.template firstIs<WorldCoordinatesMarkerNode>());
            break;
        default:
            VERIFY_NOT_REACHED(render3d);
            break;
        }
    }
    void visitPortalNode(typename NodeSequence<S>::template FirstIs<PortalNode> seq) {
        impl()->dispatchVisit(seq.subSequence());
    }
    void visitTransformNode(typename NodeSequence<S>::template FirstIs<TransformNode> seq) {
        impl()->dispatchVisit(seq.subSequence());
    }
    void visitBoundingNode(typename NodeSequence<S>::template FirstIs<BoundingNode> seq) {
        impl()->dispatchVisit(seq.subSequence());
    }
    void visitObjectNode(typename NodeSequence<S>::template FirstIs<ObjectNode>) { }
    void visitCompoundNode(typename NodeSequence<S>::template FirstIs<CompoundNode> seq) {
        for (auto subSeq : seq.first()->subSeqs())
            impl()->visitSequence(subSeq);
    }
    void visitWorldCoordinatesMarkerNode(typename NodeSequence<S>::template FirstIs<WorldCoordinatesMarkerNode> seq) {
        impl()->dispatchVisit(seq.subSequence());
    }
};

template<typename Impl>
struct ParallelVisitor {
    Impl* impl() { return static_cast<Impl*>(this); }
    void visitSequence(NodeSequence<SG> sgSeq, NodeSequence<RT> rtSeq) {
        impl()->dispatchVisit(sgSeq, rtSeq);
    }
    void dispatchVisit(NodeSequence<SG> sgSeq, NodeSequence<RT> rtSeq) {
        switch (sgSeq.frontKind()) {
        case NodeKind::Portal:
            impl()->visitPortalNode(sgSeq.firstIs<SG::PortalNode>(), rtSeq.firstIs<RT::PortalNode>());
            break;
        case NodeKind::Transform:
            impl()->visitTransformNode(sgSeq.firstIs<SG::TransformNode>(), rtSeq.firstIs<RT::TransformNode>());
            break;
        case NodeKind::Bounding:
            impl()->visitBoundingNode(sgSeq.firstIs<SG::BoundingNode>(), rtSeq.firstIs<RT::BoundingNode>());
            break;
        case NodeKind::Object:
            impl()->visitObjectNode(sgSeq.firstIs<SG::ObjectNode>(), rtSeq.firstIs<RT::ObjectNode>());
            break;
        case NodeKind::Compound:
            impl()->visitCompoundNode(sgSeq.firstIs<SG::CompoundNode>(), rtSeq.firstIs<RT::CompoundNode>());
            break;
        case NodeKind::WorldCoordinatesMarker:
            impl()->visitWorldCoordinatesMarkerNode(sgSeq.firstIs<SG::WorldCoordinatesMarkerNode>(), rtSeq.firstIs<RT::WorldCoordinatesMarkerNode>());
            break;
        default:
            VERIFY_NOT_REACHED(render3d);
            break;
        }
    }
    void visitPortalNode(NodeSequence<SG>::FirstIs<SG::PortalNode> sgSeq, NodeSequence<RT>::FirstIs<RT::PortalNode> rtSeq) {
        impl()->dispatchVisit(sgSeq.subSequence(), rtSeq.subSequence());
    }
    void visitTransformNode(NodeSequence<SG>::FirstIs<SG::TransformNode> sgSeq, NodeSequence<RT>::FirstIs<RT::TransformNode> rtSeq) {
        impl()->dispatchVisit(sgSeq.subSequence(), rtSeq.subSequence());
    }
    void visitBoundingNode(NodeSequence<SG>::FirstIs<SG::BoundingNode> sgSeq, NodeSequence<RT>::FirstIs<RT::BoundingNode> rtSeq) {
        impl()->dispatchVisit(sgSeq.subSequence(), rtSeq.subSequence());
    }
    void visitObjectNode(NodeSequence<SG>::FirstIs<SG::ObjectNode> sgSeq, NodeSequence<RT>::FirstIs<RT::ObjectNode> rtSeq) { }
    void visitCompoundNode(NodeSequence<SG>::FirstIs<SG::CompoundNode> sgSeq, NodeSequence<RT>::FirstIs<RT::CompoundNode> rtSeq) {
        auto sgSeqs = sgSeq.first()->subSeqs();
        auto rtSeqs = rtSeq.first()->subSeqs();
        VERIFY(render3d, sgSeqs.size() == rtSeqs.size());
        for (int_t i = 0; i < sgSeqs.size(); i++)
            impl()->visitSequence(sgSeqs[i], rtSeqs[i]);
    }
    void visitPortalNode(NodeSequence<SG>::FirstIs<SG::WorldCoordinatesMarkerNode> sgSeq, NodeSequence<RT>::FirstIs<RT::WorldCoordinatesMarkerNode> rtSeq) {
        impl()->dispatchVisit(sgSeq.subSequence(), rtSeq.subSequence());
    }
};

struct RTCreator : Visitor<SG, RTCreator> {
    using Base = Visitor<SG, RTCreator>;
    RT::TransformNode* worldTransform = nullptr;
    RT::PortalNode* portalStack = nullptr;
    RT::CoalescedUpdates* rtUpdates = nullptr;
    NodeSequence<RT> rtSequence;
    bool backPropagateCameraTransform = false;

    template<typename T>
    T* constructNode(const T& node) {
        return rtSequence.pushBack(node);
    }

    void visitPortalNode(NodeSequence<SG>::FirstIs<SG::PortalNode> sgSeq) {
        auto* sgNode = sgSeq.first();
        auto* rtNode = constructNode<RT::PortalNode>({});
        RT::PortalItem item { worldTransform, portalStack, sgSeq, rtNode };
        item.update();
        rtUpdates->portalItems.push_back(item);
        portalStack = rtNode;
        dispatchVisit(sgSeq.subSequence());
    }
    void visitTransformNode(NodeSequence<SG>::FirstIs<SG::TransformNode> sgSeq) {
        auto* sgNode = sgSeq.first();
        auto* rtNode = constructNode<RT::TransformNode>({});
        RT::TransformItem item { worldTransform, sgNode, rtNode };
        item.update();
        rtUpdates->transformItems.push_back(item);
        worldTransform = rtNode;
        dispatchVisit(sgSeq.subSequence());
        if (backPropagateCameraTransform)
            rtUpdates->cameraTransforms.push_back(sgNode);
    }
    void visitBoundingNode(NodeSequence<SG>::FirstIs<SG::BoundingNode> sgSeq) {
        auto* rtNode = constructNode<RT::BoundingNode>({});
        RT::BoundingItem item { worldTransform, sgSeq, rtNode };
        item.update(portalStack->stack);
        portalStack->boundingItems.push_back(item);
        if (item.output->visible)
            dispatchVisit(sgSeq.subSequence());
        else
            completeWithSentinels(sgSeq.subSequence());
    }
    void visitObjectNode(NodeSequence<SG>::FirstIs<SG::ObjectNode> sgSeq) {
        auto* sgNode = sgSeq.first();
        auto* rtNode = constructNode<RT::ObjectNode>({});
        rtUpdates->objectItems.push_back({ worldTransform, sgNode, rtNode });
    }
    void visitCompoundNode(NodeSequence<SG>::FirstIs<SG::CompoundNode> sgSeq) {
        auto* sgNode = sgSeq.first();
        auto* rtNode = constructNode<RT::CompoundNode>({ worldTransform, portalStack });
        auto replace = rtNode->beginReplace(sgNode->subSeqs().size());
        for (auto seq : sgNode->subSeqs()) {
            RTCreator creator { {}, worldTransform, portalStack, rtUpdates };
            replace.pushBack({ creator.construct(seq) });
            if (creator.backPropagateCameraTransform)
                backPropagateCameraTransform = true;
        }
        bool success = rtNode->endReplace(std::move(replace));
        VERIFY(render3d, success);
    }
    void visitWorldCoordinatesMarkerNode(NodeSequence<SG>::FirstIs<SG::WorldCoordinatesMarkerNode> sgSeq) {
        constructNode<RT::WorldCoordinatesMarkerNode>({});
        dispatchVisit(sgSeq.subSequence());
        backPropagateCameraTransform = true;
    }
    void completeWithSentinels(NodeSequence<SG> sgSeq) {
        while (!sgSeq.empty()) {
            if (sgSeq.frontKind() == NodeKind::Compound) {
                auto* rtNode = constructNode<RT::CompoundNode>({ nullptr, nullptr });
                auto alloc = rtNode->take();
                VERIFY(render3d, alloc.view().data() == nullptr);
            } else {
                RT::asSentinel(rtSequence.pushBackEmpty(sgSeq.frontKind()))->state.store(RT::NodeState::Dead, std::memory_order_relaxed);
            }
            sgSeq = sgSeq.subSequence();
        }
    }

    NodeSequence<RT> construct(NodeSequence<SG> sgSeq) {
        rtSequence = NodeSequence<RT>::allocateStorageFor(sgSeq.kinds);
        dispatchVisit(sgSeq);
        return rtSequence;
    }

    NodeSequence<RT> constructAfterExisting(NodeSequence<SG> sgSeq, NodeHandle rtNode) {
        rtSequence = NodeSequence<RT>::adoptKinds(rtNode.node, {});
        dispatchVisit(sgSeq);
        return rtSequence;
    }
};

struct RTDestroyer {
    void destroySequence(NodeSequence<RT> seq) {
        while (!seq.empty()) {
            if (seq.frontKind() == NodeKind::Compound) {
                auto alloc = seq.frontAs<RT::CompoundNode>()->take();
                for (auto subSeq : alloc.view())
                    destroySequence(subSeq);
            } else {
                RT::asSentinel(seq.front())->state.store(RT::NodeState::Dead, std::memory_order_relaxed);
            }
            seq = seq.subSequence();
        }
    }
};

inline void render(GAllocator* allocator, Framebuffer* framebuffer, Mat4x4 projection, RT::CoalescedUpdates* rtUpdates) {
    Mat4x4 viewMatrix = Mat4x4::Id;
    for (int_t i = rtUpdates->cameraTransforms.size() - 1; i >= 0; i--) {
        viewMatrix = viewMatrix * rtUpdates->cameraTransforms[i]->transform;
    }
    Mat4x4 viewProjectionMatrix = projection * viewMatrix;
    rtUpdates->rootTransform.transform = Mat4x4::invert(viewMatrix);
    for (RT::TransformItem item : rtUpdates->transformItems)
        item.update();
    for (RT::PortalItem item : rtUpdates->portalItems) {
        item.update();
        for (RT::BoundingItem bItem : item.output->boundingItems) {
            bool wasVisible = bItem.update(item.output->stack);
            bool visible = bItem.output->visible;
            if (wasVisible && !visible) {
                log("gone");
                RTDestroyer destroyer;
                destroyer.destroySequence(NodeSequence<RT>::adoptKinds(bItem.output, bItem.input.kinds));
            }
            if (visible && !wasVisible) {
                log("appear");
                // TODO: must wait at least one tick to revive nodes
                RTCreator creator { {}, bItem.worldTransform, item.output, rtUpdates };
                creator.constructAfterExisting(bItem.input, bItem.output);
            }
        }
    }
    for (int_t itemOffset = 0; itemOffset < rtUpdates->objectItems.size(); itemOffset++) {
        auto item = rtUpdates->objectItems[itemOffset];
        auto nodeState = item.output->state.load(std::memory_order_relaxed);
        if (nodeState == RT::NodeState::Dead) {
            rtUpdates->objectItems.erase(rtUpdates->objectItems.begin() + itemOffset);
            itemOffset -= 1;
            continue;
        }

        auto* node = item.input;
        auto shaderHandle = EffectShaderFactory::getHandle(node->effects);

        auto& shader = EffectShaderFactory::instance(*shaderHandle);
        shader.use();
        shader.uniformMat4x4(shader.viewProjectionUniform, viewProjectionMatrix);

        int_t slot = 0;
        for (auto& effect : node->effects) {
            auto state = shader.ordered(shader.slots[slot++]);
            auto data = Effect::data(effect.type);
            auto buffers = data.vertexBuffers.view(effect.ptr);
            for (int_t i = 0; i < buffers.size(); i++)
                state.attribPointer({ allocator->buffer(buffers[i]), data.vertexBufferElementSizes[i] });
            for (auto& tex : data.textures.view(effect.ptr))
                state.uniformTexture(tex);
            if (data.vertexUniformDesc.size > 0)
                state.uniformBuffer({ allocator->buffer(data.vertexUniformPtr.get(effect.ptr)), data.vertexUniformDesc.size });
            if (data.fragmentUniformDesc.size > 0)
                state.uniformBuffer({ allocator->buffer(data.fragmentUniformPtr.get(effect.ptr)), data.fragmentUniformDesc.size });
        }
        shader.uniformMat4x4(shader.modelUniform, item.worldTransform->transform);
        shader.attribPointer(shader.positionAttrib, node->object->vertices.vertexBuffer(allocator));
        shader.attribPointer(shader.normalAttrib, node->object->normals.vertexBuffer(allocator));
        framebuffer->draw(node->object->drawMode, node->object->indices);
        shaderHandle.LEAK();
    }
}

struct RTDumper : Visitor<RT, RTDumper> {
    using Base = Visitor<RT, RTDumper>;
    int_t indent = 0;
    std::string output;
    void info(std::string_view name, bool isLast = false) {
        output += name;
        if (!isLast)
            output += " -> ";
    }
    void visitPortalNode(NodeSequence<RT>::FirstIs<RT::PortalNode> seq) {
        info("Portal");
        Base::visitPortalNode(seq);
    }
    void visitTransformNode(NodeSequence<RT>::FirstIs<RT::TransformNode> seq) {
        info("Transform");
        Base::visitTransformNode(seq);
    }
    void visitBoundingNode(NodeSequence<RT>::FirstIs<RT::BoundingNode> seq) {
        info("Bounding");
        Base::visitBoundingNode(seq);
    }
    void visitObjectNode(NodeSequence<RT>::FirstIs<RT::ObjectNode> seq) {
        info("Object", true);
        Base::visitObjectNode(seq);
    }
    void visitCompoundNode(NodeSequence<RT>::FirstIs<RT::CompoundNode> seq) {
        info("Compound ", true);
        output += std::to_string(seq.first()->subSeqs().size());
        indent += 1;
        Base::visitCompoundNode(seq);
        indent -= 1;
    }
    void visitWorldCoordinatesMarkerNode(NodeSequence<RT>::FirstIs<RT::WorldCoordinatesMarkerNode> seq) {
        info("WorldCoordinatesMarker");
        Base::visitWorldCoordinatesMarkerNode(seq);
    }
    void visitSequence(NodeSequence<RT> seq) {
        log("{}", output);
        output = std::string(indent * 4, ' ');
        Base::visitSequence(seq);
    }
    void dump(NodeSequence<RT> seq) {
        visitSequence(seq);
        log("{}", output);
    }
};

}
