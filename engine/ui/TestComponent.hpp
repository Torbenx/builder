#pragma once

#include "GL/Buffer.hpp"
#include "resource/Resource.hpp"
#include "ui/UiComponent.hpp"
#include "util/Shaders.hpp"

class TestComponent : public UiComponent {
public:
    Resource<SingleColorShader> shader;
    Vec4 color;
    Resource<VertexBuffer> coords;
    TestComponent();
    void draw(Framebuffer&);
    void updateRect(ScreenMetrics&);
};
