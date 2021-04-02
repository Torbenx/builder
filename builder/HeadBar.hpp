#ifndef HEADER_HEAD_BAR_H
#define HEADER_HEAD_BAR_H

#include "ui/UiSceneComponent.hpp"
#include "ui/TextComponent.hpp"
#include "ui/TestComponent.hpp"

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

#endif
