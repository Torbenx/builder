#ifndef HEADER_UI_TEXTURECOMPONENT_H
#define HEADER_UI_TEXTURECOMPONENT_H

#include "UiComponent.hpp"
#include "../util/Shaders.hpp"

class TextureComponent : public UiComponent{
    public:
        std::shared_ptr<SimpleTextureShader> shader;
        Texture *texture;
        float coords[4*2];
        float texCoords[4*2];
        TextureComponent(bool createShader = true);
        void setTexRect(Rect rect);
        void draw();
        void updateRect();
        virtual void doUniforms(){}
};

#endif
