#pragma once

#include "resource/Resource.hpp"
#include "ui/UiComponent.hpp"
#include "util/Shaders.hpp"

class TextureComponent : public UiComponent {
public:
    Resource<SimpleTextureShader> shader;
    const Texture* texture = nullptr;
    Resource<VertexBuffer> coords;
    Resource<VertexBuffer> texCoords;
    TextureComponent(bool createShader = true);
    void setTexRect(Rect rect);
    void draw(Framebuffer&);
    void updateRect(ScreenMetrics&);
    virtual void doUniforms() { }
};
