#pragma once

#include "3d/Effect.hpp"
#include "3d/GraphicsObject.hpp"
#include "GL/Framebuffer.hpp"
#include "math/Box.hpp"
#include "containers/IdAllocator.hpp"
#include <set>
#include <unordered_map>

class Renderpass;

struct Shaderpass {
    Resource<id_t> shaderHandle;
    Shaderpass(Resource<id_t> shaderHandle)
        : shaderHandle(shaderHandle) { }

    static std::unique_ptr<Shaderpass> construct(Resource<id_t> shaderHandle);

    virtual void draw(GraphicsObjectCollection&, Renderpass&, GAllocator&) = 0;
    virtual void addEntry(id_t objectId, std::vector<Effect::Handle>& effects) = 0;
    virtual ~Shaderpass() { }
};
