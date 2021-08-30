#pragma once

#include "GL/Buffer.hpp"
#include "resource/Resource.hpp"
#include "ui/UiComponent.hpp"
#include "util/Shaders.hpp"

class TestComponent : public UiComponent {
public:
    Resource<SingleColorShader> shader;
    Vec4 color;
    //Resource<VertexBuffer> coords;
    StaticGData<std::array<Vec2, 4>> coords;
    TestComponent();
    void draw(Framebuffer&, GAllocator&) override;
    void updateRect(ScreenMetrics&, GAllocator&) override;
    void detach(GAllocator&) override;
};
