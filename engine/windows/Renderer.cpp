#include "../Renderer.hpp"

bool Renderer::isInisible(){
    return getState() == RENDERER_STATE_INVISIBLE;
}

bool Renderer::inBackround(){
    return getState() == RENDERER_STATE_BACKGROUND;
}

bool Renderer::inForeground(){
    return getState() == RENDERER_STATE_FOREGROUND;
}