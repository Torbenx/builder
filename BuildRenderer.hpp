#ifndef HEADER_BUILD_RENDERER_H
#define HEADER_BUILD_RENDERER_H

#include "Renderer.hpp"
#include <string>
#include "ui/TextComponent.hpp"
#include "ui/UiScene.hpp"
#include <list>

class BuildRenderer : public Renderer{
    public:
        UiScene projectChooser;
        std::list<TextComponent*> fileNameComps;
        std::string directory = "C:\\";
        
        BuildRenderer();
        void create();
        void change(int newWidth, int newHeight);
        void drawFrame();
};

#endif