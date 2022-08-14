#include "renderer.hpp"

namespace render3d {

namespace {
    struct RTCreator : Visitor<SG, RTCreator> {
        using Base = Visitor<SG, RTCreator>;
        RT::TransformNode* viewTransform = nullptr;
        RT::WorldTransformNode* worldTransform = nullptr;
        RT::PortalNode* portalStack = nullptr;
        Renderpass* renderpass = nullptr;
        NodeSequence<RT> rtSequence {};

        template<typename T>
        T* constructNode(const T& node) {
            return rtSequence.pushBack(node);
        }

        void visitPortalNode(NodeSequence<SG>::FirstIs<SG::PortalNode> sgSeq) {
            auto* rtNode = constructNode<RT::PortalNode>({});
            RT::PortalItem item { viewTransform, portalStack, sgSeq, rtNode };
            item.update();
            renderpass->portalItems.push_back(item);
            portalStack = rtNode;
            dispatchVisit(sgSeq.subSequence());
        }
        void visitTransformNode(NodeSequence<SG>::FirstIs<SG::TransformNode> sgSeq) {
            auto* sgNode = sgSeq.first();
            auto* rtNode = constructNode<RT::TransformNode>({});
            RT::TransformItem item { viewTransform, sgNode, rtNode };
            item.update();
            renderpass->transformItems.push_back(item);
            viewTransform = rtNode;
            dispatchVisit(sgSeq.subSequence());
        }
        void visitWorldTransformNode(NodeSequence<SG>::FirstIs<SG::WorldTransformNode> sgSeq) {
            auto* sgNode = sgSeq.first();
            auto* rtNode = constructNode<RT::WorldTransformNode>({});
            RT::WorldTransformItem item { viewTransform, worldTransform, sgNode, rtNode };
            item.update();
            renderpass->worldTransformItems.push_back(item);
            worldTransform = rtNode;
            dispatchVisit(sgSeq.subSequence());
        }
        void visitBoundingNode(NodeSequence<SG>::FirstIs<SG::BoundingNode> sgSeq) {
            auto* rtNode = constructNode<RT::BoundingNode>({});
            RT::BoundingItem item { viewTransform, worldTransform, sgSeq, rtNode };
            item.update(portalStack->cullPlanes);
            portalStack->boundingItems.push_back(item);
            if (item.output->visible)
                dispatchVisit(sgSeq.subSequence());
            else
                completeWithSentinels(sgSeq.subSequence());
        }
        void visitObjectNode(NodeSequence<SG>::FirstIs<SG::ObjectNode> sgSeq) {
            auto* sgNode = sgSeq.first();
            auto* rtNode = constructNode<RT::ObjectNode>({});
            renderpass->objectItems.push_back({ viewTransform, worldTransform, sgNode, rtNode });
        }
        void visitCompoundNode(NodeSequence<SG>::FirstIs<SG::CompoundNode> sgSeq) {
            auto* sgNode = sgSeq.first();
            auto* rtNode = constructNode<RT::CompoundNode>({ viewTransform, portalStack });
            auto replace = rtNode->beginReplace(sgNode->subSeqs().size());
            for (auto seq : sgNode->subSeqs()) {
                RTCreator creator { {}, viewTransform, worldTransform, portalStack, renderpass };
                replace.pushBack({ creator.construct(seq) });
            }
            bool success = rtNode->endReplace(std::move(replace));
            VERIFY(render3d, success);
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
            dispatchVisit(sgSeq.subSequence());
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
}

Renderpass::Renderpass(NodeSequence<SG> sgRoot)
    : sgRoot(sgRoot)
    , rootTransform(new RT::TransformNode { {}, Mat4x4::Id })
    , rootWorldTransform(new RT::WorldTransformNode { {}, Mat4x4::Id }) {
    RTCreator creator { {}, rootTransform.get(), rootWorldTransform.get(), nullptr, this };
    rtRoot = creator.construct(sgRoot);
}

void Renderpass::render(GAllocator* allocator, Framebuffer* framebuffer, Mat4x4 projection) {
    for (RT::TransformItem item : transformItems)
        item.update();
    for (RT::WorldTransformItem item : worldTransformItems)
        item.update();
    for (RT::PortalItem item : portalItems) {
        item.update();
        for (RT::BoundingItem bItem : item.output->boundingItems) {
            bool wasVisible = bItem.update(item.output->cullPlanes);
            bool visible = bItem.output->visible;
            if (wasVisible && !visible) {
                log("gone");
                RTDestroyer destroyer;
                destroyer.destroySequence(NodeSequence<RT>::adoptKinds(bItem.output, bItem.input.kinds));
            }
            if (visible && !wasVisible) {
                log("appear");
                // TODO: must wait at least one frame to revive nodes
                RTCreator creator { {}, bItem.viewTransform, bItem.worldTransform, item.output, this };
                creator.constructAfterExisting(bItem.input, bItem.output);
            }
        }
    }

    for (int_t itemOffset = 0; itemOffset < (int_t)objectItems.size(); itemOffset++) {
        auto item = objectItems[itemOffset];
        auto nodeState = item.output->state.load(std::memory_order_relaxed);
        if (nodeState == RT::NodeState::Dead) {
            objectItems.erase(objectItems.begin() + itemOffset);
            itemOffset -= 1;
            continue;
        }

        auto* node = item.input;
        auto shaderHandle = EffectShaderFactory::getHandle(node->effects);

        auto& shader = EffectShaderFactory::instance(*shaderHandle);
        shader.use();
        shader.uniformMat4x4(shader.viewProjectionUniform, projection * Mat4x4::invert(item.worldTransform->transform));
        shader.uniformMat4x4(shader.modelUniform, item.worldTransform->transform * item.viewTransform->transform);

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
        shader.attribPointer(shader.positionAttrib, node->object->vertices.vertexBuffer(allocator));
        shader.attribPointer(shader.normalAttrib, node->object->normals.vertexBuffer(allocator));
        framebuffer->draw(node->object->drawMode, node->object->indices);
        shaderHandle.LEAK();
    }
}

bool RT::PortalItem::update() {
    output->cullPlanes.clear();
    if (sourceStack)
        output->cullPlanes = sourceStack->cullPlanes;

    auto transform = [&](Vec3 p) { return (viewTransform->transform * Vec4(p, 1.0f)).xyz; };
    const auto& points = input.first()->points;
    if (points.empty())
        return true;
    Vec3 prev = transform(points.back());
    Vec3 eye = { 0.0f, 0.0f, 0.0f };
    for (Vec3 cur : points) {
        cur = transform(cur);
        Vec3 normal = Vec3::normalize(Vec3::cross(prev - eye, cur - eye));
        Vec4 plane = { normal, -Vec3::dot(normal, eye) };
        output->cullPlanes.push_back(plane);
        prev = cur;
    }
    return true;
}

bool RT::BoundingItem::update(const std::vector<Vec4>& planes) {
    auto bound = input.first()->bound;
    Vec3 pos = (viewTransform->transform * Vec4(bound.position, 1.0)).xyz;
    bool wasVisible = output->visible;
    bool visible = true;
    for (Vec4 plane : planes) {
        if (Vec3::dot(plane.xyz, pos) + plane.w >= bound.radius) {
            visible = false;
            break;
        }
    }
    output->visible = visible;
    return wasVisible;
}

}
