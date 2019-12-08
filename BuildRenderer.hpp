#ifndef HEADER_BUILD_RENDERER_H
#define HEADER_BUILD_RENDERER_H

#include "Renderer.hpp"
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
        
        BuildRenderer();
        void create();
        void pointer(PointerEvent pointer);
        void change(int newWidth, int newHeight);
        void setPath(std::string path);
        void drawFrame();
};

#endif