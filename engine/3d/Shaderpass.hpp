#pragma once

#include "GL/Framebuffer.hpp"
#include "3d/Effect.hpp"
#include "math/solver/LinearProgramming.hpp"
#include "util/IdAllocator.hpp"
#include <set>
#include <unordered_map>

class Shaderpass;
class Scene;

struct Renderpass {
    Framebuffer* framebuffer;
    Mat4x4 viewProjectionMatrix;
    std::vector<std::unique_ptr<Shaderpass>> passes;
};

struct Shaderpass {
    static std::unique_ptr<Shaderpass> construct(Resource<id_t> shaderHandle);

    virtual void draw(Scene&, Renderpass&) = 0;
    virtual void addEntry(int_t objectId, std::vector<EffectInstance*>& effects) = 0;
    virtual ~Shaderpass() { }
};
