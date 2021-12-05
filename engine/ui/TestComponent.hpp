#pragma once

#include "GL/Buffer.hpp"
#include "resource/Resource.hpp"
#include "ui/UiComponent.hpp"
#include "util/Shaders.hpp"

class TestComponent : public UiComponent {
public:
    Resource<SingleColorShader> shader;
    Vec4 color;
    VertexAttribHelper<Vec2> coords;
    TestComponent();
    void draw(Framebuffer&, Context<GAllocator*> = {}) override;
    void updateRect(ScreenMetrics&, Context<GAllocator*> = {}) override;
};
