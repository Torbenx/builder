#pragma once

#include "containers/ArrayView.hpp"
#include "containers/BitSet.hpp"
#include "util/log.hpp"
#include <vector>
#include <memory>

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
    bool visible(BoundingSphere) const { return true; }
};

#define IMPORT_NODES(S)                              \
    using PortalNode = typename S::PortalNode;       \
    using TransformNode = typename S::TransformNode; \
    using BoundingNode = typename S::BoundingNode;   \
    using ObjectNode = typename S::ObjectNode;       \
    using CompoundNode = typename S::CompoundNode;

enum class NodeKind {
    Invalid = 0,
    Portal,
    Transform,
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
};
template<typename S>
struct NodeSequence {
    IMPORT_NODES(S);
    // must match the NodeKind enum
    static constexpr int_t byteSizes[] = { 0, sizeof(PortalNode), sizeof(TransformNode), sizeof(BoundingNode), sizeof(ObjectNode), sizeof(CompoundNode) };
    static constexpr int_t sizeOf(NodeKind kind) { return byteSizes[(int_t)kind]; }
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
        NodeKind kind = kinds.back();
        sizeInBytes -= sizeOf(kind);
        return { kind, begin + sizeInBytes };
    }
    template<typename T>
    T* pushBack(const T& node) {
        T* r = std::construct_at(reinterpret_cast<T*>(begin + sizeInBytes), node);
        sizeInBytes += sizeof(T);
        kinds.pushBack(T::NODE_KIND);
        return r;
    }

    template<typename T>
    struct FirstIs : NodeSequence {
        T* first() const { return reinterpret_cast<T*>(begin); }
    };
    template<typename T>
    struct LastIs : NodeSequence {
        T* last() const { return reinterpret_cast<T*>(begin + sizeInBytes - sizeof(T)); }
    };

    template<typename T>
    FirstIs<T> firstIs() const { return { *this }; }
    template<typename T>
    LastIs<T> lastIs() const { return { *this }; }
};

struct SG /* Scene Graph */ {
    struct alignas(8) PortalNode {
        static constexpr NodeKind NODE_KIND = NodeKind::Portal;
        bool isDirty = false;
        // portal data...
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
        std::vector<NodeSequence<SG>::LastIs<SG::ObjectNode>> objectSeqs;
        std::vector<NodeSequence<SG>::LastIs<SG::CompoundNode>> compoundSeqs;
        std::vector<Update> updates;

        void clearUpdates() { updates.clear(); }
        void addObjectSequence(NodeSequence<SG>::LastIs<SG::ObjectNode> seq) {
            updates.push_back({ UpdateKind::Insertion, true, (int_t)objectSeqs.size() });
            objectSeqs.push_back(seq);
        }
        void addCompoundSequence(NodeSequence<SG>::LastIs<SG::CompoundNode> seq) {
            updates.push_back({ UpdateKind::Insertion, false, (int_t)compoundSeqs.size() });
            compoundSeqs.push_back(seq);
        }
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
struct StaticNodeSequence {
    static constexpr NodeKindSequence kinds { { NodeTypes::NODE_KIND... } };
    std::tuple<NodeTypes...> storage;
    operator NodeSequence<SG>::LastIs<typename detail::LastPackParameter<NodeTypes...>::type>() {
        return { (std::byte*)&storage, kinds, (uint32_t)sizeof(decltype(storage)) };
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
    };
    struct TransformItem {
        RT::TransformNode* sourceTransform = nullptr;
        SG::TransformNode* input = nullptr;
        RT::TransformNode* output = nullptr;
    };
    struct BoundingItem {
        RT::TransformNode* worldTransform = nullptr;
        NodeSequence<SG>::FirstIs<SG::BoundingNode> input;
        RT::BoundingNode* output = nullptr;
    };
    struct ObjectItem {
        RT::TransformNode* worldTransform = nullptr;
        SG::ObjectNode* input = nullptr;
        RT::ObjectNode* output = nullptr;
    };

    struct PortalNode {
        static constexpr NodeKind NODE_KIND = NodeKind::Portal;
        CullStack stack;
        bool culled = false;
        std::vector<BoundingItem> boundingItems;
    };
    struct TransformNode {
        static constexpr NodeKind NODE_KIND = NodeKind::Transform;

        Mat4x4 transform;
    };
    struct BoundingNode {
        static constexpr NodeKind NODE_KIND = NodeKind::Bounding;

        bool culled = false;
    };
    struct ObjectNode {
        static constexpr NodeKind NODE_KIND = NodeKind::Object;
    };
    struct CompoundNode {
        static constexpr NodeKind NODE_KIND = NodeKind::Compound;

        RT::TransformNode* worldTransform;
        RT::PortalNode* portalStack;
        std::vector<NodeSequence<RT>::LastIs<RT::ObjectNode>> objectSeqs;
        std::vector<NodeSequence<RT>::LastIs<RT::CompoundNode>> compoundSeqs;
    };

    struct CoalescedUpdates {
        std::vector<TransformItem> transformItems;
        std::vector<PortalItem> portalItems;
        std::vector<ObjectItem> objectItems;
    };
};

template<typename S, typename Impl>
struct Visitor {
    IMPORT_NODES(S);
    Impl* impl() { return static_cast<Impl*>(this); }
    void visitObjectSequence(typename NodeSequence<S>::template LastIs<ObjectNode> seq) {
        impl()->dispatchVisit(seq);
    }
    void visitCompoundSequence(typename NodeSequence<S>::template LastIs<CompoundNode> seq) {
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
        default:
            std::abort();
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
        for (auto subSeq : seq.first()->objectSeqs)
            impl()->visitObjectSequence(subSeq);
        for (auto subSeq : seq.first()->CompoundSeqs)
            impl()->visitCompoundSequence(subSeq);
    }
};

struct RTCreator : Visitor<SG, RTCreator> {
    RT::TransformNode* worldTransform = nullptr;
    RT::PortalNode* portalStack = nullptr;
    RT::CoalescedUpdates* rtUpdates = nullptr;
    NodeSequence<RT> rtSequence;

    template<typename T>
    T* constructNode(const T& node) {
        return rtSequence.pushBack(node);
    }

    void visitPortalNode(NodeSequence<SG>::FirstIs<SG::PortalNode> sgSeq) {
        auto* sgNode = sgSeq.first();
        auto* rtNode = constructNode<RT::PortalNode>({});
        rtUpdates->portalItems.push_back({ worldTransform, portalStack, sgSeq, rtNode });
        portalStack = rtNode;
        dispatchVisit(sgSeq.subSequence());
    }
    void visitTransformNode(NodeSequence<SG>::FirstIs<SG::TransformNode> sgSeq) {
        auto* sgNode = sgSeq.first();
        auto* rtNode = constructNode<RT::TransformNode>({});
        VDUMP((void*)sgNode);
        rtNode->transform = worldTransform->transform * sgNode->transform;
        rtUpdates->transformItems.push_back({ worldTransform, sgNode, rtNode });
        worldTransform = rtNode;
        dispatchVisit(sgSeq.subSequence());
    }
    void visitBoundingNode(NodeSequence<SG>::FirstIs<SG::BoundingNode> sgSeq) {
        auto* rtNode = constructNode<RT::BoundingNode>({});
        portalStack->boundingItems.push_back({ worldTransform, sgSeq, rtNode });
        dispatchVisit(sgSeq.subSequence());
    }
    void visitObjectNode(NodeSequence<SG>::FirstIs<SG::ObjectNode> sgSeq) {
        auto* sgNode = sgSeq.first();
        auto* rtNode = constructNode<RT::ObjectNode>({});
        rtUpdates->objectItems.push_back({ worldTransform, sgNode, rtNode });
    }
    void visitCompoundNode(NodeSequence<SG>::FirstIs<SG::CompoundNode> sgSeq) {
        auto* sgNode = sgSeq.first();
        auto* rtNode = constructNode<RT::CompoundNode>({ worldTransform, portalStack });
        VDUMP((void*)sgNode);
        rtNode->objectSeqs.reserve(sgNode->objectSeqs.size());
        for (auto seq : sgNode->objectSeqs) {
            RTCreator creator { {}, worldTransform, portalStack, rtUpdates };
            rtNode->objectSeqs.push_back({ creator.construct(seq) });
        }
        rtNode->compoundSeqs.reserve(sgNode->compoundSeqs.size());
        for (auto seq : sgNode->compoundSeqs) {
            RTCreator creator { {}, worldTransform, portalStack, rtUpdates };
            rtNode->compoundSeqs.push_back({ creator.construct(seq) });
        }
    }

    NodeSequence<RT> construct(NodeSequence<SG> sgSeq) {
        rtSequence = NodeSequence<RT>::allocateStorageFor(sgSeq.kinds);
        dispatchVisit(sgSeq);
        return rtSequence;
    }
};

struct RTUpdateChecker {
    RT::CoalescedUpdates* rtUpdates = nullptr;
    void checkCompoundSequence(NodeSequence<SG>::LastIs<SG::CompoundNode> sgSeq, NodeSequence<RT>::LastIs<RT::CompoundNode> rtSeq) {
        checkCompoundNode(sgSeq.last(), rtSeq.last());
    }
    void checkCompoundNode(SG::CompoundNode* sgNode, RT::CompoundNode* rtNode) {
        for (auto update : sgNode->updates) {
            switch (update.kind) {
            case SG::CompoundNode::UpdateKind::Insertion: {
                RTCreator creator { {}, rtNode->worldTransform, rtNode->portalStack, rtUpdates };
                auto sgSeq = update.isObjectSeq ? (NodeSequence<SG>)sgNode->objectSeqs[update.index] : (NodeSequence<SG>)sgNode->compoundSeqs[update.index];
                auto rtSeq = creator.construct(sgSeq);
                if (update.isObjectSeq)
                    rtNode->objectSeqs.push_back({ rtSeq });
                else
                    rtNode->compoundSeqs.push_back({ rtSeq });
                break;
            }
            }
        }

        VERIFY(render3d, sgNode->compoundSeqs.size() == rtNode->compoundSeqs.size());
        for (int_t i = 0; i < sgNode->compoundSeqs.size(); i++)
            checkCompoundSequence(sgNode->compoundSeqs[i], rtNode->compoundSeqs[i]);
    }
};

inline void render(GAllocator* allocator, Framebuffer* framebuffer, Mat4x4 projection, RT::CoalescedUpdates* rtUpdates) {
    for (RT::ObjectItem item : rtUpdates->objectItems) {
        auto* node = item.input;
        auto shaderHandle = EffectShaderFactory::getHandle(node->effects);

        auto& shader = EffectShaderFactory::instance(*shaderHandle);
        shader.use();
        shader.uniformMat4x4(shader.viewProjectionUniform, projection * item.worldTransform->transform);

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
        shader.uniformMat4x4(shader.modelUniform, node->object->modelMatrix);
        shader.attribPointer(shader.positionAttrib, node->object->vertices.vertexBuffer(allocator));
        shader.attribPointer(shader.normalAttrib, node->object->normals.vertexBuffer(allocator));
        framebuffer->draw(node->object->drawMode, node->object->indices);
        shaderHandle.LEAK();
    }
}

}
