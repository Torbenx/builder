#ifndef HEADER_FILE_EXPLORER_H
#define HEADER_FILE_EXPLORER_H

#include "ui/UiRenderer.hpp"
#include <string>
#include "ui/TextComponent.hpp"
#include <vector>

class FileExplorer : public UiRenderer{
    public:
        std::vector<TextComponent*> fileNameComps;
        bool hasMetrics = false;
        std::string path;
        void *renderer;
    
        FileExplorer(void *renderer);
        void setPath(std::string path);
        void onChange(ScreenMetrics metrics);
        void onClick(int id);
        void drawFrame();
        ~FileExplorer();
};

#endif