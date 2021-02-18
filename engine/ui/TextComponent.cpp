#include "TextComponent.hpp"
#include "../util/log.hpp"

TextComponent::TextComponent() : TextureComponent(TEXT_TEXTURE_GLSL_HEADER, TEXT_TEXTURE_GLSL_SAMPLER, "texture2D", ""){
    setTexRect(Rect::bltr(1.0f, 0.0f, 0.0f, 1.0f)); //invert y-direction
}

void TextComponent::updateContent(){
    if(textFormat){
        delete textFormat;
    }
    textFormat = new TextFormat(text, noBreaks ? -1 : ceil(TypedSize::gl(rect.getWidth()).toPixels(metrics.x)), textSize.toPixels(metrics.y));
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
    if(!noBreaks)updateContent();
    textTexture = new TextTexture();
    textTexture->init(ceil(TypedSize::gl(rect.getWidth()).toPixels(metrics.x)), ceil(TypedSize::gl(rect.getHeight()).toPixels(metrics.y)));
    textTexture->drawText(*textFormat, Vec2(0.0f, 0.0f), textColor, true);
    texture = &textTexture->texture;
}

TypedSizeXY TextComponent::getContentWidth(){
    return TypedSize::pixels(textFormat->getWidth()).asXSize();
}

TypedSizeXY TextComponent::getContentHeight(){
    return TypedSize::pixels(textFormat->getHeight()).asYSize();
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