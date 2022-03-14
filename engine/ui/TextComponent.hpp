#pragma once

#include "ui/TextureComponent.hpp"
#include "util/text.hpp"

class TextComponent : public TextureComponent {
public:
    TextFormat* textFormat = nullptr;
    TextTexture* textTexture = nullptr;
    Vec4 textColor = Vec4(0.0f, 0.0f, 0.0f, 1.0f);
    std::string text;
    TypedSize textSize;
    bool noBreaks = false;
    TextComponent();
    void draw(Framebuffer&, GAllocator*) override;
    void updateRect(ScreenMetrics&, GAllocator*) override;
    void updateContent(ScreenMetrics&, GAllocator*) override;
    TypedSizeXY getContentWidth() override;
    TypedSizeXY getContentHeight() override;
    ~TextComponent();
};
