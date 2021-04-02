#pragma once

#include <ui/UiSceneComponent.hpp>
#include <ui/TextComponent.hpp>
#include <ui/TestComponent.hpp>

class HeadBar : public UiSceneComponent{
    public:
        void *renderer;
        TextComponent title;
        TextComponent subTitle;
        TestComponent box;
        bool hasMetrics = false;
        ScreenMetrics metrics;

        HeadBar(void *renderer);
        void setPath(std::string path);
        void updateRect(ScreenMetrics&);
        void draw(Framebuffer&);
        ~HeadBar();
};
