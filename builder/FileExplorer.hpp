#pragma once

#include <ui/UiSceneComponent.hpp>
#include <ui/TestComponent.hpp>
#include <ui/TextComponent.hpp>
#include <string>
#include <vector>

class FileExplorer : public UiSceneComponent{
    public:
        std::vector<TextComponent*> fileNameComps;
        std::string path;
        void *renderer;
        TestComponent background;
        GAllocator& allocator;

        FileExplorer(void *renderer);
        void setPath(std::string path);
        void onClick(int id) override;
        void draw(Framebuffer&, GAllocator&) override;
        ~FileExplorer();
};
