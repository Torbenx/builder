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

struct CullItem {
    Vec3 position;
    float radius;
    uint32_t id;
};

enum class NodeKind {
    Portal,
    Transform,
    Items,
    Compound,
};

namespace CG /* Cull Graph */ {
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
    struct ItemsNode : Node {
        struct Update {
            bool isAddition = true;
            bool isDynamic;
        };
        std::vector<CullItem> staticItems;
        std::vector<CullItem> dynamicItems;
        std::vector<Update> updates;

        ItemsNode()
            : Node(NodeKind::Items) { }

        void addStaticItem(CullItem item) {
            staticItems.push_back(item);
            updates.push_back({ .isDynamic = false });
        }
        void addDynamicItem(CullItem item) {
            dynamicItems.push_back(item);
            updates.push_back({ .isDynamic = true });
        }
        void clearUpdates() { updates.clear(); }
    };
    struct CompoundNode : Node {
        struct Update {
            bool isAddition = true;
        };
        std::vector<Node*> children;
        std::vector<Update> updates;

        CompoundNode()
            : Node(NodeKind::Compound) { }

        void addChild(Node* node) {
            children.push_back(node);
            updates.push_back({});
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
        void visitItemsNode(ItemsNode*, Args...) { }
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
            case NodeKind::Items:
                return impl()->visitItemsNode((ItemsNode*)node, args...);
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
    struct ItemsNode : Node {
        struct Update {
            enum class Kind {
                Added,
                BecameVisible,
                BecameInvisible,
            };
            Kind kind;
            uint32_t id;
        };
        BitSet staticItems;
        BitSet dynamicItems;
        std::vector<Update> updates;

        ItemsNode()
            : Node(NodeKind::Items) { }
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
        void visitItemsNode(ItemsNode*, Args...) { }
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
            case NodeKind::Items:
                return impl()->visitItemsNode((ItemsNode*)node, args...);
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
    bool visible(CullItem) const { return true; }
};

struct RTUpdater {
    CullStack cullStack;
    Mat4x4 fullTransform { true };
    RT::Node* updatePortalNode(CG::PortalNode* cgNode, RT::PortalNode* rtNode) {
        if (!rtNode)
            rtNode = new RT::PortalNode;
        rtNode->child = dispatchUpdate(cgNode->child, rtNode->child);
        return rtNode;
    }
    RT::Node* updateTransformNode(CG::TransformNode* cgNode, RT::TransformNode* rtNode) {
        if (!rtNode)
            rtNode = new RT::TransformNode;

        CullStack oldCullStack = std::move(cullStack);
        // cullStack = cgNode->transform * oldCullStack;
        Mat4x4 oldFullTransform = fullTransform;
        fullTransform = fullTransform * cgNode->transform;
        rtNode->fullTransform = fullTransform;

        rtNode->child = dispatchUpdate(cgNode->child, rtNode->child);

        cullStack = std::move(oldCullStack);
        fullTransform = oldFullTransform;

        return rtNode;
    }
    void cullArray(RT::ItemsNode* out, ArrayView<const CullItem> items, BitSet& pastResults) {
        pastResults.reserve(items.size());
        for (int_t i = 0; i < items.size(); i++) {
            bool isVisible = cullStack.visible(items[i]);
            bool wasVisible = pastResults.get(i);
            if (isVisible && !wasVisible)
                out->updates.push_back({ RT::ItemsNode::Update::Kind::BecameVisible, items[i].id });
            else if (!isVisible && wasVisible)
                out->updates.push_back({ RT::ItemsNode::Update::Kind::BecameInvisible, items[i].id });
            pastResults.set(i, isVisible);
        }
    }
    RT::Node* updateItemsNode(CG::ItemsNode* cgNode, RT::ItemsNode* rtNode) {
        if (!rtNode)
            rtNode = new RT::ItemsNode;
        rtNode->updates.clear();

        int_t staticCount = 0;
        int_t dynamicCount = 0;
        for (auto update : cgNode->updates) {
            uint32_t id;
            if (update.isDynamic)
                id = cgNode->dynamicItems[cgNode->dynamicItems.size() - (++dynamicCount)].id;
            else
                id = cgNode->staticItems[cgNode->staticItems.size() - (++staticCount)].id;
            rtNode->updates.push_back({ RT::ItemsNode::Update::Kind::Added, id });
        }
        cullArray(rtNode, cgNode->staticItems, rtNode->staticItems);
        cullArray(rtNode, cgNode->dynamicItems, rtNode->dynamicItems);
        return rtNode;
    }
    RT::Node* updateCompoundNode(CG::CompoundNode* cgNode, RT::CompoundNode* rtNode) {
        if (!rtNode)
            rtNode = new RT::CompoundNode;
        for (auto update : cgNode->updates) {
            if (update.isAddition)
                rtNode->children.push_back(nullptr);
        }
        VERIFY(render3d, cgNode->children.size() == rtNode->children.size());
        for (int_t i = 0; i < cgNode->children.size(); i++) {
            rtNode->children[i] = dispatchUpdate(cgNode->children[i], rtNode->children[i]);
        }
        return rtNode;
    }
    RT::Node* dispatchUpdate(CG::Node* cgNode, RT::Node* rtNode) {
        if (rtNode)
            VERIFY(render3d, cgNode->kind == rtNode->kind);
        switch (cgNode->kind) {
        case NodeKind::Portal:
            return updatePortalNode((CG::PortalNode*)cgNode, (RT::PortalNode*)rtNode);
        case NodeKind::Transform:
            return updateTransformNode((CG::TransformNode*)cgNode, (RT::TransformNode*)rtNode);
        case NodeKind::Items:
            return updateItemsNode((CG::ItemsNode*)cgNode, (RT::ItemsNode*)rtNode);
        case NodeKind::Compound:
            return updateCompoundNode((CG::CompoundNode*)cgNode, (RT::CompoundNode*)rtNode);
        default:
            VERIFY_NOT_REACHED(render3d);
            return nullptr;
        }
    }
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
    void visitItemsNode(RT::ItemsNode* node) {
        Base::visitItemsNode(node);
        delete node;
    }
    void visitCompoundNode(RT::CompoundNode* node) {
        Base::visitCompoundNode(node);
        delete node;
    }
};

struct Instance {
    struct Object {
        const GraphicsObject* object;
        std::vector<Effect::Handle> effects;
    };
    IdAllocator<> objectIdAllocator;
    std::vector<Object> objects;

    uint32_t addObject(Object object) {
        uint32_t id = objectIdAllocator.allocateId();
        if (id >= objects.size())
            objects.resize(id + 1);
        objects[id] = std::move(object);
        return id;
    }
};

struct LightAttacher : RT::Visitor<LightAttacher> {
    using Base = RT::Visitor<LightAttacher>;

    Instance* instance = nullptr;
    Effect::Handle effect;

    void visitItemsNode(RT::ItemsNode* node) {
        for (auto update : node->updates) {
            if (update.kind == RT::ItemsNode::Update::Kind::Added) {
                instance->objects[update.id].effects.push_back(effect);
            }
        }
    }
};

struct Renderer : RT::Visitor<Renderer> {
    using Base = RT::Visitor<Renderer>;

    std::vector<std::pair<uint32_t, Mat4x4*>> objects;
    Mat4x4* currentTransform = nullptr;

    void visitTransformNode(RT::TransformNode* node) {
        currentTransform = &node->fullTransform;
        Base::visitTransformNode(node);
    }

    void visitItemsNode(RT::ItemsNode* node) {
        for (auto update : node->updates) {
            if (update.kind == RT::ItemsNode::Update::Kind::Added)
                objects.push_back({ update.id, currentTransform });
        }
    }

    void render(Instance* instance, GAllocator* allocator, Framebuffer* framebuffer, Mat4x4 projection) {
        for (auto [id, transform] : objects) {
            auto& object = instance->objects[id];
            auto shaderHandle = EffectShaderFactory::getHandle(object.effects);

            auto& shader = EffectShaderFactory::instance(*shaderHandle);
            shader.use();
            shader.uniformMat4x4(shader.viewProjectionUniform, projection * (*transform));

            int_t slot = 0;
            for (auto& effect : object.effects) {
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
            shader.uniformMat4x4(shader.modelUniform, object.object->modelMatrix);
            shader.attribPointer(shader.positionAttrib, object.object->vertices.vertexBuffer(allocator));
            shader.attribPointer(shader.normalAttrib, object.object->normals.vertexBuffer(allocator));
            framebuffer->draw(object.object->drawMode, object.object->indices);
            shaderHandle.LEAK();
        }
    }
};

}
