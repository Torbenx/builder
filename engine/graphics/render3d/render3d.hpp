#pragma once

#include "containers/ArrayView.hpp"
#include "containers/BitSet.hpp"
#include "util/log.hpp"
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

enum class NodeKind {
    Portal,
    Transform,
    Bounding,
    Object,
    Compound,
};

namespace SG /* Scene Graph */ {
    struct Node {
        NodeKind kind;
        Node(NodeKind kind)
            : kind(kind) { }
    };
    struct PortalNode : Node {
        Node* child = nullptr;
        bool isDirty = false;
        // portal data...
        PortalNode()
            : Node(NodeKind::Portal) { }
    };
    struct TransformNode : Node {
        Node* child = nullptr;
        Mat4x4 transform;
        TransformNode()
            : Node(NodeKind::Transform) { }
    };
    struct BoundingNode : Node {
        Node* child = nullptr;
        BoundingSphere bound;
        BoundingNode()
            : Node(NodeKind::Bounding) { }
    };
    struct ObjectNode : Node {
        const GraphicsObject* object;
        std::vector<Effect::Handle> effects;
        ObjectNode()
            : Node(NodeKind::Object) { }
    };
    struct CompoundNode : Node {
        struct Update {
            enum class Kind {
                Insertion,
                Update,
            };
            Kind kind;
            int_t index;
        };
        std::vector<Node*> children;
        std::vector<Update> updates;

        CompoundNode()
            : Node(NodeKind::Compound) { }

        void addChild(Node* node) {
            int_t index = children.size();
            children.push_back(node);
            updates.push_back({ Update::Kind::Insertion, index });
        }
        void clearUpdates() { updates.clear(); }
    };

    template<typename Impl, typename RetT = void, typename... Args>
    struct Visitor {
        Impl* impl() { return static_cast<Impl*>(this); }
        void visitPortalNode(PortalNode* node, Args... args) {
            impl()->dispatchVisit(node->child, args...);
        }
        void visitTransformNode(TransformNode* node, Args... args) {
            impl()->dispatchVisit(node->child, args...);
        }
        void visitBoundingNode(BoundingNode* node, Args... args) {
            impl()->dispatchVisit(node->child, args...);
        }
        void visitObjectNode(ObjectNode*, Args...) { }
        void visitCompoundNode(CompoundNode* node, Args... args) {
            for (Node* child : node->children)
                impl()->dispatchVisit(child, args...);
        }
        RetT dispatchVisit(Node* node, Args... args) {
            switch (node->kind) {
            case NodeKind::Portal:
                return impl()->visitPortalNode((PortalNode*)node, args...);
            case NodeKind::Transform:
                return impl()->visitTransformNode((TransformNode*)node, args...);
            case NodeKind::Bounding:
                return impl()->visitBoundingNode((BoundingNode*)node, args...);
            case NodeKind::Object:
                return impl()->visitObjectNode((ObjectNode*)node, args...);
            case NodeKind::Compound:
                return impl()->visitCompoundNode((CompoundNode*)node, args...);
            default:
                VERIFY_NOT_REACHED(render3d);
                std::abort();
            }
        }
    };
}

namespace RT /* Rendering Tree */ {
    struct Node {
        NodeKind kind;
        Node(NodeKind kind)
            : kind(kind) { }
    };
    struct PortalNode : Node {
        Node* child = nullptr;
        PortalNode()
            : Node(NodeKind::Portal) { }
    };
    struct TransformNode : Node {
        Node* child = nullptr;
        Mat4x4 fullTransform;
        TransformNode()
            : Node(NodeKind::Transform) { }
    };
    struct BoundingNode : Node {
        Node* child = nullptr;
        bool isVisible = false;
        BoundingNode()
            : Node(NodeKind::Bounding) { }
    };
    struct ObjectNode : Node {
        ObjectNode()
            : Node(NodeKind::Object) { }
        const GraphicsObject* object;
        std::vector<Effect::Handle> effects;
    };
    struct CompoundNode : Node {
        std::vector<Node*> children;
        CompoundNode()
            : Node(NodeKind::Compound) { }
    };

    template<typename Impl, typename RetT = void, typename... Args>
    struct Visitor {
        Impl* impl() { return static_cast<Impl*>(this); }
        void visitPortalNode(PortalNode* node, Args... args) {
            impl()->dispatchVisit(node->child, args...);
        }
        void visitTransformNode(TransformNode* node, Args... args) {
            impl()->dispatchVisit(node->child, args...);
        }
        void visitBoundingNode(BoundingNode* node, Args... args) {
            if (node->isVisible)
                impl()->dispatchVisit(node->child, args...);
        }
        void visitObjectNode(ObjectNode*, Args...) { }
        void visitCompoundNode(CompoundNode* node, Args... args) {
            for (Node* child : node->children)
                impl()->dispatchVisit(child, args...);
        }
        RetT dispatchVisit(Node* node, Args... args) {
            switch (node->kind) {
            case NodeKind::Portal:
                return impl()->visitPortalNode((PortalNode*)node, args...);
            case NodeKind::Transform:
                return impl()->visitTransformNode((TransformNode*)node, args...);
            case NodeKind::Bounding:
                return impl()->visitBoundingNode((BoundingNode*)node, args...);
            case NodeKind::Object:
                return impl()->visitObjectNode((ObjectNode*)node, args...);
            case NodeKind::Compound:
                return impl()->visitCompoundNode((CompoundNode*)node, args...);
            default:
                VERIFY_NOT_REACHED(render3d);
                std::abort();
            }
        }
    };
}

struct CullStack {
    bool visible(BoundingSphere) const { return true; }
};

struct RTDestroyer : RT::Visitor<RTDestroyer> {
    using Base = RT::Visitor<RTDestroyer>;
    void visitPortalNode(RT::PortalNode* node) {
        Base::visitPortalNode(node);
        delete node;
    }
    void visitTransformNode(RT::TransformNode* node) {
        Base::visitTransformNode(node);
        delete node;
    }
    void visitBoundingNode(RT::BoundingNode* node) {
        Base::visitBoundingNode(node);
        if (node->isVisible)
            delete node;
    }
    void visitObjectNode(RT::ObjectNode* node) {
        Base::visitObjectNode(node);
        delete node;
    }
    void visitCompoundNode(RT::CompoundNode* node) {
        Base::visitCompoundNode(node);
        delete node;
    }
};

struct RTUpdater {
    CullStack cullStack;
    Mat4x4 fullTransform { true };
    RT::Node* updatePortalNode(SG::PortalNode* sgNode, RT::PortalNode* rtNode) {
        if (!rtNode)
            rtNode = new RT::PortalNode;
        rtNode->child = dispatchUpdate(sgNode->child, rtNode->child);
        return rtNode;
    }
    RT::Node* updateTransformNode(SG::TransformNode* sgNode, RT::TransformNode* rtNode) {
        if (!rtNode)
            rtNode = new RT::TransformNode;

        CullStack oldCullStack = std::move(cullStack);
        // cullStack = sgNode->transform * oldCullStack;
        Mat4x4 oldFullTransform = fullTransform;
        fullTransform = fullTransform * sgNode->transform;
        rtNode->fullTransform = fullTransform;

        rtNode->child = dispatchUpdate(sgNode->child, rtNode->child);

        cullStack = std::move(oldCullStack);
        fullTransform = oldFullTransform;

        return rtNode;
    }
    RT::Node* updateBoundingNode(SG::BoundingNode* sgNode, RT::BoundingNode* rtNode) {
        if (!rtNode)
            rtNode = new RT::BoundingNode;
        bool isVisible = cullStack.visible(sgNode->bound);
        if (!isVisible && rtNode->isVisible) {
            RTDestroyer dest;
            dest.dispatchVisit(rtNode->child);
        }
        rtNode->isVisible = isVisible;
        if (isVisible)
            rtNode->child = dispatchUpdate(sgNode->child, rtNode->child);

        return rtNode;
    }
    RT::Node* updateObjectNode(SG::ObjectNode* sgNode, RT::ObjectNode* rtNode) {
        if (!rtNode)
            rtNode = new RT::ObjectNode;
        rtNode->effects = sgNode->effects;
        rtNode->object = sgNode->object;
        return rtNode;
    }
    RT::Node* updateCompoundNode(SG::CompoundNode* sgNode, RT::CompoundNode* rtNode) {
        if (!rtNode)
            rtNode = new RT::CompoundNode;
        for (auto update : sgNode->updates) {
            if (update.kind == SG::CompoundNode::Update::Kind::Insertion)
                rtNode->children.push_back(nullptr);
        }
        VERIFY(render3d, sgNode->children.size() == rtNode->children.size());
        for (int_t i = 0; i < sgNode->children.size(); i++) {
            rtNode->children[i] = dispatchUpdate(sgNode->children[i], rtNode->children[i]);
        }
        return rtNode;
    }
    RT::Node* dispatchUpdate(SG::Node* sgNode, RT::Node* rtNode) {
        if (rtNode)
            VERIFY(render3d, sgNode->kind == rtNode->kind);
        switch (sgNode->kind) {
        case NodeKind::Portal:
            return updatePortalNode((SG::PortalNode*)sgNode, (RT::PortalNode*)rtNode);
        case NodeKind::Transform:
            return updateTransformNode((SG::TransformNode*)sgNode, (RT::TransformNode*)rtNode);
        case NodeKind::Bounding:
            return updateBoundingNode((SG::BoundingNode*)sgNode, (RT::BoundingNode*)rtNode);
        case NodeKind::Object:
            return updateObjectNode((SG::ObjectNode*)sgNode, (RT::ObjectNode*)rtNode);
        case NodeKind::Compound:
            return updateCompoundNode((SG::CompoundNode*)sgNode, (RT::CompoundNode*)rtNode);
        default:
            VERIFY_NOT_REACHED(render3d);
            return nullptr;
        }
    }
};

struct Renderer : RT::Visitor<Renderer> {
    using Base = RT::Visitor<Renderer>;
    Renderer(GAllocator* allocator, Framebuffer* framebuffer, Mat4x4 projection)
        : allocator(allocator), framebuffer(framebuffer), projection(projection) { }

    Mat4x4 currentTransform;
    GAllocator* allocator;
    Framebuffer* framebuffer;
    Mat4x4 projection;

    void visitTransformNode(RT::TransformNode* node) {
        currentTransform = node->fullTransform;
        Base::visitTransformNode(node);
    }

    void visitObjectNode(RT::ObjectNode* node) {
        auto shaderHandle = EffectShaderFactory::getHandle(node->effects);

        auto& shader = EffectShaderFactory::instance(*shaderHandle);
        shader.use();
        shader.uniformMat4x4(shader.viewProjectionUniform, projection * currentTransform);

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
};

}
