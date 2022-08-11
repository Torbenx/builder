#pragma once

#include "trees.hpp"
#include "visitors.hpp"

namespace render3d {

struct Renderpass {
    NodeSequence<SG> sgRoot;
    NodeSequence<RT> rtRoot;
    std::unique_ptr<RT::TransformNode> rootTransform;
    std::vector<SG::TransformNode*> cameraTransforms;
    std::vector<RT::ObjectItem> objectItems;
    Vec3 worldEye;
    Renderpass();
    void updateRootNodes();
    void render(GAllocator* allocator, Framebuffer* framebuffer, Mat4x4 projection);
};

struct Renderer {
    std::vector<RT::TransformItem> transformItems;
    std::vector<RT::PortalItem> portalItems;
    std::vector<Renderpass> renderpasses;
    Renderpass* createRenderpass(NodeSequence<SG> sgRoot);
    void update();
};

}
