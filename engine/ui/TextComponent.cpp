#include "TextComponent.hpp"

TextComponent::TextComponent() : TextureComponent(TEXT_TEXTURE_GLSL_HEADER, TEXT_TEXTURE_GLSL_SAMPLER, "texture2D"){
    setTexRect(Rect::bltr(1.0f, 0.0f, 0.0f, 1.0f)); //invert y-direction
}

void TextComponent::updateText(){
    if(textFormat){
        delete textFormat;
    }
    textFormat = new TextFormat(text, toPixelWidth(rect.getWidth()), toPixelHeight(textSize));
    if(textTexture){
        textTexture->drawText(*textFormat, Vec2(0.0f, 0.0f), textColor, true);
    }
}

void TextComponent::updateRect(){
    TextureComponent::updateRect();
    if(textTexture){
        delete textTexture;
        textTexture = nullptr;
    }
    updateText();
    textTexture = new TextTexture();
    textTexture->init(toPixelWidth(rect.getWidth()), toPixelHeight(rect.getHeight()));
    textTexture->drawText(*textFormat, Vec2(0.0f, 0.0f), textColor, true);
    texture = &textTexture->texture;
}

float TextComponent::getTextWidth(){
    return toGLWidth(textFormat->getWidth());
}

float TextComponent::getTextHeight(){
    return toGLHeight(textFormat->getHeight());
}

void TextComponent::draw(){
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    TextureComponent::draw();
    glDisable(GL_BLEND);
}

TextComponent::~TextComponent(){
    if(textFormat){
        delete textFormat;
    }
    if(textTexture){
        delete textTexture;
    }
}