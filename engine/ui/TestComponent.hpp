#ifndef HEADER_UI_TESTCOMPONENT_H
#define HEADER_UI_TESTCOMPONENT_H

#include "UiComponent.hpp"
#include "../graphic/Shader.hpp"

class TestComponent : public UiComponent{
    public:
        Shader shader;
        Mat4x4 transform;
        Vec3 color;
        float coords[4*2];
        TestComponent(Vec3 color);
        void setTransform(Mat4x4 transform);
        void draw();
        Rect getBoundary();
};

#endif