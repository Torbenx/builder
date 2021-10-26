#pragma once

#include "util/fullscreen.hpp"
#include "GL/glwrapper.hpp"
#include <string>
#include <list>

struct FullscreenConfigImpl{
    std::string adapterName;
    DEVMODE deviceMode;
};

void engine_goForeground();
bool engine_goBackground();
std::list<std::string> getDisplayList();
FullscreenConfig getFullscreenConfig(std::string adapterName, int width, int height, int refreshRate);
