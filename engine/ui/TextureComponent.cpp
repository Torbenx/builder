#include "ui/TextureComponent.hpp"

TextureComponent::TextureComponent(bool createShader)
    : coords(VertexBuffer::alloc<Vec2>(4)), texCoords(VertexBuffer::alloc<Vec2>(4)) {
    if (createShader)
        shader = Shaders::texture();
    setTexRect(Rect::bltr(0.0f, 0.0f, 1.0f, 1.0f));
}

void TextureComponent::draw(Framebuffer& framebuffer, GAllocator*) {
    if (!texture)
        return;
    shader->use();
    shader->attribPointer(shader->positionAttrib, coords.value);
    shader->attribPointer(shader->texCoordAttrib, texCoords.value);
    shader->uniformTexture(shader->textureUniform, *texture);
    doUniforms();
    framebuffer.draw(DrawMode::TriangleStrip, 4);
}

void TextureComponent::updateRect(ScreenMetrics&, GAllocator*) {
    coords.value.update(rect.corners());
}

void TextureComponent::setTexRect(Rect rect) {
    texCoords.value.update(rect.corners());
}
