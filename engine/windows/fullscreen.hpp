#ifndef HEADER_WINDOWS_FULLSCREEN_H
#define HEADER_WINDOWS_FULLSCREEN_H

#include <Windows.h>
#include <string>
#include <list>

class FullscreenConfig{
    public:
        static FullscreenConfig DEFAULT;
        FullscreenConfig();
        FullscreenConfig(std::string adapterName, DEVMODE deviceMode);
        std::string adapterName;
        DEVMODE deviceMode;
};

void engine_goForeground();
bool engine_goBackground();
std::list<std::string> getDisplayList();
FullscreenConfig getFullscreenConfig(std::string adapterName, int width, int height, int refreshRate);

#endif