#include "ui/TextureComponent.hpp"

TextureComponent::TextureComponent(bool createShader) : coords(4), texCoords(4){
    if(createShader)
        shader = Shaders::texture();
    setTexRect(Rect::bltr(0.0f, 0.0f, 1.0f, 1.0f));
}

void TextureComponent::draw(Framebuffer& framebuffer){
    shader->use();
    shader->attribPointer(shader->positionAttrib, coords);
    shader->attribPointer(shader->texCoordAttrib, texCoords);
    shader->uniformTexture(shader->textureUniform, *texture, 0);
    doUniforms();
    framebuffer.draw(DrawMode::TriangleStrip, 4);
}

void TextureComponent::updateRect(ScreenMetrics&){
    coords.update(rect.corners());
}

void TextureComponent::setTexRect(Rect rect){
    texCoords.update(rect.corners());
}
