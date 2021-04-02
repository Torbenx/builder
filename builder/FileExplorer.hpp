#pragma once

#include <ui/UiSceneComponent.hpp>
#include <ui/TestComponent.hpp>
#include <ui/TextComponent.hpp>
#include <string>
#include <vector>

class FileExplorer : public UiSceneComponent{
    public:
        std::vector<TextComponent*> fileNameComps;
        bool hasMetrics = false;
        ScreenMetrics metrics;
        std::string path;
        void *renderer;
        TestComponent background;

        FileExplorer(void *renderer);
        void setPath(std::string path);
        void updateRect(ScreenMetrics&);
        void onClick(int id);
        void draw(Framebuffer&);
        ~FileExplorer();
};
