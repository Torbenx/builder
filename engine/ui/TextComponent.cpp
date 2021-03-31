#include "TextComponent.hpp"
#include "../util/log.hpp"
#include "../util/time.hpp"

TextComponent::TextComponent() : TextureComponent(false){
    setTexRect(Rect::bltr(1.0f, 0.0f, 0.0f, 1.0f)); //invert y-direction
    shader = Shaders::text();
}

void TextComponent::updateContent(){
    if(textFormat){
        delete textFormat;
    }
    textFormat = new TextFormat(text, noBreaks ? -1 : ceil(TypedSize::gl(rect.getWidth()).toPixels(metrics.x)), textSize.toPixels(metrics.y));
    if(textTexture){
        delete textTexture;
    }
    textTexture = new TextTexture(textFormat->getWidth(), textFormat->getHeight());
    textTexture->drawText(*textFormat, Vec2(0.0f, 0.0f), textColor, true);
    texture = textTexture;
//#ifdef PLATFORM_WINDOWS
    texture->setTexParameter(GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_BORDER, GL_CLAMP_TO_BORDER);
//#else
//    texture->setTexParameter(GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
//#endif
}

void TextComponent::updateRect(){
    TextureComponent::updateRect();
    if(!noBreaks)updateContent();
    Rect texRect = Rect::bltr(rect.getHeight() / getContentHeight().toGL(metrics), 0.0f, 0.0f, rect.getWidth() / getContentWidth().toGL(metrics));
    setTexRect(texRect);
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
