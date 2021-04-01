#ifndef HEADER_UI_CUBE_MAP_COMPONENT_H
#define HEADER_UI_CUBE_MAP_COMPONENT_H

#include "../util/Shaders.hpp"
#include "UiComponent.hpp"

class CubeMapComponent : public UiComponent{
    public:
        std::shared_ptr<SimpleCubeMapShader> shader;
        CubeMap *cubeMap;
        float coords[4*2];
        float texCoords[4*3];
        CubeMapComponent(bool createShader=true);
        void draw(Framebuffer&);
        void updateRect();
        virtual void doUniforms(){}
        virtual void positiveX(Rect rect);
        virtual void negativeX(Rect rect);
        virtual void positiveY(Rect rect);
        virtual void negativeY(Rect rect);
        virtual void positiveZ(Rect rect);
        virtual void negativeZ(Rect rect);
};

#endif
