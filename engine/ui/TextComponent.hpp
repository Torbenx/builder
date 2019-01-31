#ifndef HEADER_UI_TEXTCOMPONENT_H
#define HEADER_UI_TEXTCOMPONENT_H

#include "TextureComponent.hpp"
#include "text.hpp"

class TextComponent : public TextureComponent{
    public:
        TextFormat *textFormat = nullptr;
        TextTexture *textTexture = nullptr;
        Vec4 textColor = Vec4(0.0f, 0.0f, 0.0f, 1.0f);
        std::string text;
        float textSize;
        TextComponent();
        void draw();
        void updateRect();
        void updateText();
        float getTextWidth();
        float getTextHeight();
        ~TextComponent();
};

#endif