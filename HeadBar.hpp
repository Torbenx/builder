#ifndef HEADER_HEAD_BAR_H
#define HEADER_HEAD_BAR_H

#include "ui/UiRenderer.hpp"
#include "ui/TextComponent.hpp"

class HeadBar : public UiRenderer{
    public:
        void *renderer;
        TextComponent title;
        bool hasMetrics = false;
        
        HeadBar(void *renderer);
        void setPath(std::string path);
        void onChange(ScreenMetrics metrics);
        void drawFrame();
        ~HeadBar();
};

#endif