#pragma once

#include <Renderer.hpp>
#include "FileExplorer.hpp"
#include "HeadBar.hpp"
#include <string>
#include <vector>

class BuildRenderer : public Renderer{
    public:
        FileExplorer explorer;
        HeadBar head;
        std::vector<std::string> files;
        bool isProject = false;
        UiScene scene;

        BuildRenderer();
        void pointer(PointerEvent pointer) override;
        void change(int newWidth, int newHeight) override;
        void setPath(std::string path);
        void drawFrame() override;
};
