#include "TextureComponent.hpp"

TextureComponent::TextureComponent(bool createShader){
    if(createShader)
        shader = Shaders::texture();
    setTexRect(Rect::bltr(0.0f, 0.0f, 1.0f, 1.0f));
}

void TextureComponent::draw(Framebuffer&){
    shader->use();
    shader->attribPointer(shader->positionAttrib, 2, coords);
    shader->attribPointer(shader->texCoordAttrib, 2, texCoords);
    shader->uniformTexture(shader->textureUniform, *texture, 0);
    doUniforms();
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void TextureComponent::updateRect(){
    coords[0] = rect.getLeft();
    coords[1] = rect.getBottom();

    coords[2] = rect.getRight();
    coords[3] = rect.getBottom();

    coords[4] = rect.getLeft();
    coords[5] = rect.getTop();

    coords[6] = rect.getRight();
    coords[7] = rect.getTop();
}

void TextureComponent::setTexRect(Rect rect){
    texCoords[0] = rect.getLeft();
    texCoords[1] = rect.getBottom();

    texCoords[2] = rect.getRight();
    texCoords[3] = rect.getBottom();

    texCoords[4] = rect.getLeft();
    texCoords[5] = rect.getTop();

    texCoords[6] = rect.getRight();
    texCoords[7] = rect.getTop();
}
