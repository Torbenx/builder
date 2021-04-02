#pragma once

#include "util/Shaders.hpp"
#include "UiComponent.hpp"

class CubeMapComponent : public UiComponent{
    public:
        std::shared_ptr<SimpleCubeMapShader> shader;
        CubeMap *cubeMap;
        Buffer<Vec2> coords;
        Buffer<Vec3> texCoords;
        CubeMapComponent(bool createShader=true);
        void draw(Framebuffer&);
        void updateRect(ScreenMetrics&);
        virtual void doUniforms(){}
        virtual void positiveX(Rect rect);
        virtual void negativeX(Rect rect);
        virtual void positiveY(Rect rect);
        virtual void negativeY(Rect rect);
        virtual void positiveZ(Rect rect);
        virtual void negativeZ(Rect rect);
};
