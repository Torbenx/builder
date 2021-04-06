#pragma once

#include "ui/UiComponent.hpp"
#include "util/Shaders.hpp"

class TextureComponent : public UiComponent{
    public:
        std::shared_ptr<SimpleTextureShader> shader;
        Texture *texture;
        VertexBuffer coords;
        VertexBuffer texCoords;
        TextureComponent(bool createShader = true);
        void setTexRect(Rect rect);
        void draw(Framebuffer&);
        void updateRect(ScreenMetrics&);
        virtual void doUniforms(){}
};
