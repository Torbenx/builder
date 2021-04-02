#ifndef HEADER_UI_TESTCOMPONENT_H
#define HEADER_UI_TESTCOMPONENT_H

#include "UiComponent.hpp"
#include "../util/Shaders.hpp"
#include "../graphic/Buffer.hpp"

class TestComponent : public UiComponent{
    public:
        std::shared_ptr<SingleColorShader> shader;
        Vec4 color;
        Buffer<Vec2> coords;
        TestComponent();
        void draw(Framebuffer&);
        void updateRect(ScreenMetrics&);
};

#endif
