#ifndef HEADER_FILE_EXPLORER_H
#define HEADER_FILE_EXPLORER_H

#include "ui/UiSceneComponent.hpp"
#include <string>
#include "ui/TextComponent.hpp"
#include <vector>

class FileExplorer : public UiSceneComponent{
    public:
        std::vector<TextComponent*> fileNameComps;
        bool hasMetrics = false;
        std::string path;
        void *renderer;
    
        FileExplorer(void *renderer);
        void setPath(std::string path);
        void updateRect();
        void onClick(int id);
        void draw();
        ~FileExplorer();
};

#endif