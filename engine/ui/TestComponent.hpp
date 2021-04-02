#pragma once

#include "ui/UiComponent.hpp"
#include "util/Shaders.hpp"
#include "GL/Buffer.hpp"

class TestComponent : public UiComponent{
    public:
        std::shared_ptr<SingleColorShader> shader;
        Vec4 color;
        Buffer<Vec2> coords;
        TestComponent();
        void draw(Framebuffer&);
        void updateRect(ScreenMetrics&);
};
