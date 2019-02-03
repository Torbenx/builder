#ifndef HEADER_UI_TESTCOMPONENT_H
#define HEADER_UI_TESTCOMPONENT_H

#include "UiComponent.hpp"
#include "../graphic/Shader.hpp"

class TestComponent : public UiComponent{
    public:
        Shader shader;
        Vec4 color;
        float coords[4*2];
        TestComponent();
        void draw();
        void updateRect();
};

#endif