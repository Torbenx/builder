#pragma once

#include "ui/UiRenderer.hpp"
#include "ui/TextComponent.hpp"

class Body : public UiRenderer{
    public:
        void *renderer;
        bool hasMetrics = false;

        Body(void *renderer);
        void setPath(std::string path);
        void onChange(ScreenMetrics metrics);
        void drawFrame();
        ~Body();
};
