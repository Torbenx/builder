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
        TypedSize textSize;
        bool noBreaks = false;
        TextComponent();
        void draw();
        void updateRect();
        void updateContent();
        TypedSizeXY getContentWidth();
        TypedSizeXY getContentHeight();
        ~TextComponent();
};

#endif