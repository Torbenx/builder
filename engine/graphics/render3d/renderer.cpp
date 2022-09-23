#include "renderer.hpp"
#include "visitors.hpp"

namespace render3d {

namespace {
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
    inline Sentinel* asSentinel(NodeHandle node) {
        return (Sentinel*)node.node;
    }

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
            bool update(const Mat4x4& invProjection);
        };
        struct TransformItem {
            RT::TransformNode* sourceTransform = nullptr;
            SG::TransformNode* input = nullptr;
            RT::TransformNode* output = nullptr;
            void update();
        };
        struct WorldTransformItem {
            RT::TransformNode* viewTransform = nullptr;
            RT::WorldTransformNode* sourceWorldTransform = nullptr;
            SG::WorldTransformNode* input = nullptr;
            RT::WorldTransformNode* output = nullptr;
            void update();
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
        struct SubpassNode : Sentinel {
            static constexpr NodeKind NODE_KIND = NodeKind::Subpass;
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
    };

    void RT::TransformItem::update() {
        output->transform = sourceTransform->transform * input->transform;
    }
    void RT::WorldTransformItem::update() {
        output->transform
            = sourceWorldTransform->transform
            * viewTransform->transform
            * input->transform
            * Mat4x4::invert(viewTransform->transform);
    }
    bool RT::PortalItem::update(const Mat4x4& invProjection) {
        output->cullPlanes.clear();
        if (sourceStack)
            output->cullPlanes = sourceStack->cullPlanes;

        auto transform = [&](Vec3 p) { return (invProjection * viewTransform->transform * Vec4(p, 1.0f)).xyz; };
        const auto& points = input.first()->points;
        if (points.empty())
            return true;
        Vec3 prev = transform(points.back());
        Vec3 eye = { 0.0f, 0.0f, 0.0f };
        for (Vec3 cur : points) {
            cur = transform(cur);
            Vec3 normal = Vec3::normalize(Vec3::cross(prev - eye, cur - eye));
            Vec4 plane = { normal, -Vec3::dot(normal, eye) };
            output->cullPlanes.push_back(plane * invProjection);
            prev = cur;
        }
        return true;
    }
    bool RT::BoundingItem::update(const std::vector<Vec4>& planes) {
        auto bound = input.first()->bound;
        Vec4 pos = (viewTransform->transform * Vec4(bound.position, 1.0));
        bool wasVisible = output->visible;
        bool visible = true;
        for (Vec4 plane : planes) {
            if (Vec3::dot(plane.xyz, pos.xyz) + plane.w * pos.w >= bound.radius) {
                visible = false;
                break;
            }
        }
        output->visible = visible;
        return wasVisible;
    }

    struct RenderpassImpl : Renderpass {
        NodeSequence<SG> sgRoot;
        NodeSequence<RT> rtRoot;
        std::unique_ptr<RT::TransformNode> rootTransform;
        std::unique_ptr<RT::WorldTransformNode> rootWorldTransform;
        std::vector<RT::TransformItem> transformItems;
        std::vector<RT::WorldTransformItem> worldTransformItems;
        std::vector<RT::PortalItem> portalItems;
        std::vector<RT::ObjectItem> objectItems;
        Mat4x4 invProjection;
        RenderpassImpl(NodeSequence<SG> sgRoot);
        void update(const Mat4x4& projection);
        void render(GAllocator* allocator, Framebuffer* framebuffer);
        void dumpRenderingTree() const;
    };

    struct RTCreator : Visitor<SG, RTCreator> {
        using Base = Visitor<SG, RTCreator>;
        RT::TransformNode* viewTransform = nullptr;
        RT::WorldTransformNode* worldTransform = nullptr;
        RT::PortalNode* portalStack = nullptr;
        RenderpassImpl* renderpass = nullptr;
        NodeSequence<RT> rtSequence {};

        template<typename T>
        T* constructNode(const T& node) {
            return rtSequence.pushBack(node);
        }

        void visitPortalNode(NodeSequence<SG>::FirstIs<SG::PortalNode> sgSeq) {
            auto* rtNode = constructNode<RT::PortalNode>({});
            RT::PortalItem item { viewTransform, portalStack, sgSeq, rtNode };
            item.update(renderpass->invProjection);
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
        void visitSubpassNode(NodeSequence<SG>::FirstIs<SG::SubpassNode> sgSeq) {
            auto* rtNode = constructNode<RT::SubpassNode>({});
            dispatchVisit(sgSeq.subSequence());
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
                    asSentinel(rtSequence.pushBackEmpty(sgSeq.frontKind()))->state.store(NodeState::Dead, std::memory_order_relaxed);
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
                    asSentinel(seq.front())->state.store(NodeState::Dead, std::memory_order_relaxed);
                }
                seq = seq.subSequence();
            }
        }
    };

    RenderpassImpl::RenderpassImpl(NodeSequence<SG> sgRoot)
        : sgRoot(sgRoot)
        , rootTransform(new RT::TransformNode { {}, Mat4x4::Id })
        , rootWorldTransform(new RT::WorldTransformNode { {}, Mat4x4::Id }) { }

    void RenderpassImpl::update(const Mat4x4& projection) {
        Mat4x4 invProjection = Mat4x4::invert(projection);
        rootTransform->transform = projection;
        rootWorldTransform->transform = invProjection;
        this->invProjection = invProjection;
        if (rtRoot.size() == 0) {
            RTCreator creator { {}, rootTransform.get(), rootWorldTransform.get(), nullptr, this };
            rtRoot = creator.construct(sgRoot);
            return;
        }
        for (RT::TransformItem item : transformItems)
            item.update();
        for (RT::WorldTransformItem item : worldTransformItems)
            item.update();
        for (RT::PortalItem item : portalItems) {
            item.update(invProjection);
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
    }

    void RenderpassImpl::render(GAllocator* allocator, Framebuffer* framebuffer) {
        for (int_t itemOffset = 0; itemOffset < (int_t)objectItems.size(); itemOffset++) {
            auto item = objectItems[itemOffset];
            auto nodeState = item.output->state.load(std::memory_order_relaxed);
            if (nodeState == NodeState::Dead) {
                objectItems.erase(objectItems.begin() + itemOffset);
                itemOffset -= 1;
                continue;
            }

            auto* node = item.input;

            std::sort(node->effects.begin(), node->effects.end(), [](Effect::Handle first, Effect::Handle second) {
                return first.type.data < second.type.data;
            });
            auto shaderHandle = EffectShaderFactory::getHandle(node->effects);

            auto& shader = EffectShaderFactory::instance(*shaderHandle);
            shader.use();
            shader.uniformMat4x4(shader.viewProjectionUniform, Mat4x4::invert(item.worldTransform->transform));
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
        void visitWorldTransformNode(NodeSequence<RT>::FirstIs<RT::WorldTransformNode> seq) {
            info("WorldTransform");
            Base::visitWorldTransformNode(seq);
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

    void RenderpassImpl::dumpRenderingTree() const {
        RTDumper dumper;
        dumper.dump(rtRoot);
    }
}

Renderpass* Renderpass::create(NodeSequence<SG> sgRoot) {
    return new RenderpassImpl { sgRoot };
}
void Renderpass::update(const Mat4x4& projection) {
    static_cast<RenderpassImpl*>(this)->update(projection);
}
void Renderpass::render(GAllocator* allocator, Framebuffer* framebuffer) {
    static_cast<RenderpassImpl*>(this)->render(allocator, framebuffer);
}
void Renderpass::dumpRenderingTree() const {
    static_cast<const RenderpassImpl*>(this)->dumpRenderingTree();
}
Renderpass::~Renderpass() {
    static_cast<RenderpassImpl*>(this)->~RenderpassImpl();
}

}
