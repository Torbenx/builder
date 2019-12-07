#ifndef HEADER_BUILD_RENDERER_H
#define HEADER_BUILD_RENDERER_H

#include "Renderer.hpp"
#include "FileExplorer.hpp"
#include <string>

class BuildRenderer : public Renderer{
    public:
        FileExplorer explorer;
        
        BuildRenderer();
        void create();
        void pointer(PointerEvent pointer);
        void change(int newWidth, int newHeight);
        void drawFrame();
};

#endif