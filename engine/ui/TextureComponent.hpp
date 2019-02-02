#ifndef HEADER_UI_TEXTURECOMPONENT_H
#define HEADER_UI_TEXTURECOMPONENT_H

#include "UiComponent.hpp"
#include "../graphic/Shader.hpp"

class TextureComponent : public UiComponent{
    public:
        Shader shader;
        Texture *texture;
        float coords[4*2];
        float texCoords[4*2];
        TextureComponent();
        TextureComponent(std::string header, std::string sampler, std::string texFunc, std::string modColor);
        void setTexRect(Rect rect);
        void draw();
        void updateRect();
        virtual void doUniforms(){}
};

#endif