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
    void draw(Framebuffer&) override;
    void updateRect(ScreenMetrics&) override;
    virtual void doUniforms() { }
};
