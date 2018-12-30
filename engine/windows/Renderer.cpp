#include "../Renderer.hpp"

bool Renderer::isInvisible(){
    return getState() == RENDERER_STATE_INVISIBLE;
}

bool Renderer::isBackground(){
    return getState() == RENDERER_STATE_BACKGROUND;
}

bool Renderer::isForeground(){
    return getState() == RENDERER_STATE_FOREGROUND;
}