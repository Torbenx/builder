#include "TextComponent.hpp"
#include "../util/log.hpp"
#include "../util/time.hpp"

TextComponent::TextComponent() : TextureComponent(false){
    setTexRect(Rect::bltr(1.0f, 0.0f, 0.0f, 1.0f)); //invert y-direction
    shader = Shaders::text();
}

void TextComponent::updateContent(ScreenMetrics& metrics){
    if(textFormat){
        delete textFormat;
    }
    textFormat = new TextFormat(text, noBreaks ? -1 : ceil(TypedSize::gl(rect.width()).toPixels(metrics.x)), textSize.toPixels(metrics.y));
    if(textTexture){
        delete textTexture;
    }
    textTexture = new TextTexture(textFormat->getWidth(), textFormat->getHeight());
    textTexture->drawText(*textFormat, Vec2(0.0f, 0.0f), textColor, true);
    texture = textTexture;
    texture->bind();
    texture->wrappingMode(Texture::WrappingMode::ClampBorder, Texture::WrappingMode::ClampBorder);
}

void TextComponent::updateRect(ScreenMetrics& metrics){
    TextureComponent::updateRect(metrics);
    if(!noBreaks) updateContent(metrics);
    Rect texRect = Rect::bltr(rect.height() / getContentHeight().toGL(metrics), 0.0f, 0.0f, rect.width() / getContentWidth().toGL(metrics));
    setTexRect(texRect);
}

TypedSizeXY TextComponent::getContentWidth(){
    if(textFormat)
        return TypedSize::pixels(textFormat->getWidth()).asXSize();
    return TypedSize::nan.asXSize();
}

TypedSizeXY TextComponent::getContentHeight(){
    if(textFormat)
        return TypedSize::pixels(textFormat->getHeight()).asYSize();
    return TypedSize::nan.asYSize();
}

void TextComponent::draw(Framebuffer& framebuffer){
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    TextureComponent::draw(framebuffer);
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
