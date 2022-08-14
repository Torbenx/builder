#pragma once

#include "3d/Renderpass.hpp"
#include "NodeSequence.hpp"
#include "math/Mat4x4.hpp"

namespace render3d {

struct Renderpass;

struct BoundingSphere {
    Vec3 position;
    float radius;
};

struct SG /* Scene Graph */ {
    struct alignas(8) PortalNode {
        static constexpr NodeKind NODE_KIND = NodeKind::Portal;
        bool b = false;
        std::vector<Vec3> points;
    };
    struct alignas(8) TransformNode {
        static constexpr NodeKind NODE_KIND = NodeKind::Transform;
        Mat4x4 transform;
    };
    struct alignas(8) WorldTransformNode {
        static constexpr NodeKind NODE_KIND = NodeKind::WorldTransform;
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
};

struct RT /* Rendering Tree */ {
    struct PortalNode;
    struct TransformNode;
    struct WorldTransformNode;
    struct BoundingNode;
    struct ObjectNode;
    struct CompoundNode;

    struct PortalItem {
        RT::TransformNode* viewTransform = nullptr;
        RT::PortalNode* sourceStack = nullptr;
        NodeSequence<SG>::FirstIs<SG::PortalNode> input;
        RT::PortalNode* output = nullptr;
        bool update();
    };
    struct TransformItem {
        RT::TransformNode* sourceTransform = nullptr;
        SG::TransformNode* input = nullptr;
        RT::TransformNode* output = nullptr;
        void update() {
            output->transform = sourceTransform->transform * input->transform;
        }
    };
    struct WorldTransformItem {
        RT::TransformNode* viewTransform = nullptr;
        RT::WorldTransformNode* sourceWorldTransform = nullptr;
        SG::WorldTransformNode* input = nullptr;
        RT::WorldTransformNode* output = nullptr;
        void update() {
            output->transform
                = sourceWorldTransform->transform
                * viewTransform->transform
                * input->transform
                * Mat4x4::invert(viewTransform->transform);
        }
    };
    struct BoundingItem {
        RT::TransformNode* viewTransform = nullptr;
        RT::WorldTransformNode* worldTransform = nullptr;
        NodeSequence<SG>::FirstIs<SG::BoundingNode> input;
        RT::BoundingNode* output = nullptr;
        bool update(const std::vector<Vec4>& planes);
    };
    struct ObjectItem {
        RT::TransformNode* viewTransform = nullptr;
        RT::WorldTransformNode* worldTransform = nullptr;
        SG::ObjectNode* input = nullptr;
        RT::ObjectNode* output = nullptr;
    };

    enum class NodeState {
        Alive,
        Dead,
    };
    struct alignas(8) Sentinel {
        std::atomic<NodeState> state = NodeState::Alive;
        Sentinel() { }
        Sentinel(const Sentinel& other)
            : state(other.state.load(std::memory_order_relaxed)) { }
        Sentinel(NodeState state)
            : state(state) { }
    };
    struct PortalNode : Sentinel {
        static constexpr NodeKind NODE_KIND = NodeKind::Portal;
        std::vector<Vec4> cullPlanes;
        // ConvexPolygon screenSpacePolygon;
        std::vector<BoundingItem> boundingItems;
    };
    struct TransformNode : Sentinel {
        static constexpr NodeKind NODE_KIND = NodeKind::Transform;
        Mat4x4 transform;
    };
    struct WorldTransformNode : Sentinel {
        static constexpr NodeKind NODE_KIND = NodeKind::WorldTransform;
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

        RT::TransformNode* viewTransform;
        RT::PortalNode* portalStack;
        std::atomic<uintptr_t> subSequenceStorage = (uintptr_t)0 | (uintptr_t)1;

        CompoundNode(RT::TransformNode* viewTransform, RT::PortalNode* portalStack)
            : viewTransform(viewTransform), portalStack(portalStack) { }
        CompoundNode(const CompoundNode& other)
            : viewTransform(other.viewTransform), portalStack(other.portalStack), subSequenceStorage(other.subSequenceStorage.load(std::memory_order_relaxed)) { }

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
    static inline Sentinel* asSentinel(NodeHandle node) {
        return (Sentinel*)node.node;
    }
};

}
