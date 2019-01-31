#include "TextureComponent.hpp"

TextureComponent::TextureComponent() : TextureComponent("", "sampler2D", "texture2D"){
    
}

TextureComponent::TextureComponent(std::string header, std::string sampler, std::string texFunc) : 
shader(
"attribute vec4 a_position;\n"
"attribute vec2 a_texPosition;\n"
"varying vec2 v_texPosition;\n"
"void main(){\n"
"   v_texPosition = a_texPosition;\n"
"   gl_Position = a_position;\n"
"}",
header+
"uniform "+sampler+" u_texture;\n"
"varying vec2 v_texPosition;\n"
"void main(){\n"
"   gl_FragColor = "+texFunc+"(u_texture, v_texPosition);\n"
"}"){
    setTexRect(Rect::bltr(0.0f, 0.0f, 1.0f, 1.0f));
}

void TextureComponent::draw(){
    shader.init();
    shader.use();
    shader.attribPointer("a_position", 2, coords);
    shader.attribPointer("a_texPosition", 2, texCoords);
    shader.uniformTexture("u_texture", *texture, 0);
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