#pragma once

#include "3d/Renderpass.hpp"
#include "NodeSequence.hpp"
#include "math/Mat4x4.hpp"

namespace render3d {

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

struct Renderpass {
    static Renderpass* create(NodeSequence<SG> sgRoot);
    void update(const Mat4x4& projection);
    void render(GAllocator* allocator, Framebuffer* framebuffer);
    void dumpRenderingTree() const;
    ~Renderpass();
};

}
