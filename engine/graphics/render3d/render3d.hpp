#pragma once

#include "containers/ArrayView.hpp"
#include "containers/BitSet.hpp"
#include "containers/Tuple.hpp"
#include "util/log.hpp"
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
    template<typename T>
    static constexpr int_t sizeOf() { return sizeOf(T::NODE_KIND); }
    template<typename T>
    static constexpr bool isSizeOfValid() {
        if (std::is_empty_v<T>)
            return sizeOf<T>() == 0;
        return sizeOf<T>() == sizeof(T);
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
        sizeInBytes += sizeOf<T>();
        kinds.pushBack(T::NODE_KIND);
        return r;
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
    constexpr int_t byteSizes[] = { 0, sizeof(PortalNode), sizeof(TransformNode), sizeof(BoundingNode), sizeof(ObjectNode), sizeof(CompoundNode), 0 };
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

    struct PortalNode {
        static constexpr NodeKind NODE_KIND = NodeKind::Portal;
        CullStack stack;
        std::vector<BoundingItem> boundingItems;
    };
    struct TransformNode {
        static constexpr NodeKind NODE_KIND = NodeKind::Transform;

        Mat4x4 transform;
    };
    struct BoundingNode {
        static constexpr NodeKind NODE_KIND = NodeKind::Bounding;
        bool visible = false;
    };
    struct ObjectNode {
        static constexpr NodeKind NODE_KIND = NodeKind::Object;
        bool b = false;
    };
    struct CompoundNode {
        static constexpr NodeKind NODE_KIND = NodeKind::Compound;

        RT::TransformNode* worldTransform;
        RT::PortalNode* portalStack;
        std::vector<NodeSequence<RT>::LastIs<RT::ObjectNode>> objectSeqs;
        std::vector<NodeSequence<RT>::LastIs<RT::CompoundNode>> compoundSeqs;
    };
    struct WorldCoordinatesMarkerNode {
        static constexpr NodeKind NODE_KIND = NodeKind::WorldCoordinatesMarker;
    };

    struct CoalescedUpdates {
        RT::TransformNode rootTransform { Mat4x4::Id };
        std::vector<SG::TransformNode*> cameraTransforms;
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
        impl()->visitSequence(seq);
    }
    void visitCompoundSequence(typename NodeSequence<S>::template LastIs<CompoundNode> seq) {
        impl()->visitSequence(seq);
    }
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
        for (auto subSeq : seq.first()->objectSeqs)
            impl()->visitObjectSequence(subSeq);
        for (auto subSeq : seq.first()->compoundSeqs)
            impl()->visitCompoundSequence(subSeq);
    }
    void visitWorldCoordinatesMarkerNode(typename NodeSequence<S>::template FirstIs<WorldCoordinatesMarkerNode> seq) {
        impl()->dispatchVisit(seq.subSequence());
    }
};

struct RTCreator : Visitor<SG, RTCreator> {
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
    }
    void visitObjectNode(NodeSequence<SG>::FirstIs<SG::ObjectNode> sgSeq) {
        auto* sgNode = sgSeq.first();
        auto* rtNode = constructNode<RT::ObjectNode>({});
        rtUpdates->objectItems.push_back({ worldTransform, sgNode, rtNode });
    }
    void visitCompoundNode(NodeSequence<SG>::FirstIs<SG::CompoundNode> sgSeq) {
        auto* sgNode = sgSeq.first();
        auto* rtNode = constructNode<RT::CompoundNode>({ worldTransform, portalStack });
        rtNode->objectSeqs.reserve(sgNode->objectSeqs.size());
        for (auto seq : sgNode->objectSeqs) {
            RTCreator creator { {}, worldTransform, portalStack, rtUpdates };
            rtNode->objectSeqs.push_back({ creator.construct(seq) });
            if (creator.backPropagateCameraTransform)
                backPropagateCameraTransform = true;
        }
        rtNode->compoundSeqs.reserve(sgNode->compoundSeqs.size());
        for (auto seq : sgNode->compoundSeqs) {
            RTCreator creator { {}, worldTransform, portalStack, rtUpdates };
            rtNode->compoundSeqs.push_back({ creator.construct(seq) });
            if (creator.backPropagateCameraTransform)
                backPropagateCameraTransform = true;
        }
    }
    void visitWorldCoordinatesMarkerNode(NodeSequence<SG>::FirstIs<SG::WorldCoordinatesMarkerNode> sgSeq) {
        constructNode<RT::WorldCoordinatesMarkerNode>({});
        dispatchVisit(sgSeq.subSequence());
        backPropagateCameraTransform = true;
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
        for (int_t i = 0; i < sgNode->compoundSeqs.size(); i++) {
            if (rtNode->compoundSeqs[i].size() == rtNode->compoundSeqs[i].size())
                checkCompoundSequence(sgNode->compoundSeqs[i], rtNode->compoundSeqs[i]);
            else
                log("skipped!");
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
            }
            if (visible && !wasVisible) {
                log("appear");
            }
        }
    }
    for (RT::ObjectItem item : rtUpdates->objectItems) {
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
        output += std::to_string(seq.first()->objectSeqs.size());
        output += "/";
        output += std::to_string(seq.first()->compoundSeqs.size());
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
