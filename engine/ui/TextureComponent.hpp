#pragma once

#include "ui/UiComponent.hpp"
#include "util/Shaders.hpp"

class TextureComponent : public UiComponent{
    public:
        std::shared_ptr<SimpleTextureShader> shader;
        Texture *texture;
        Buffer<Vec2> coords;
        Buffer<Vec2> texCoords;
        TextureComponent(bool createShader = true);
        void setTexRect(Rect rect);
        void draw(Framebuffer&);
        void updateRect(ScreenMetrics&);
        virtual void doUniforms(){}
};
