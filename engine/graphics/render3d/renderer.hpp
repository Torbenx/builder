#pragma once

#include "trees.hpp"
#include "visitors.hpp"

namespace render3d {

struct Renderpass {
    NodeSequence<SG> sgRoot;
    NodeSequence<RT> rtRoot;
    std::unique_ptr<RT::TransformNode> rootTransform;
    std::unique_ptr<RT::WorldTransformNode> rootWorldTransform;
    std::vector<RT::TransformItem> transformItems;
    std::vector<RT::WorldTransformItem> worldTransformItems;
    std::vector<RT::PortalItem> portalItems;
    std::vector<RT::ObjectItem> objectItems;
    Renderpass(NodeSequence<SG> sgRoot);
    void render(GAllocator* allocator, Framebuffer* framebuffer, Mat4x4 projection);
};

}
