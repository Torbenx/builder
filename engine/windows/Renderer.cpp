#include "Renderer.hpp"

bool RendererState::isInvisible(){
    return getState() == RENDERER_STATE_INVISIBLE;
}

bool RendererState::isBackground(){
    return getState() == RENDERER_STATE_BACKGROUND;
}

bool RendererState::isForeground(){
    return getState() == RENDERER_STATE_FOREGROUND;
}
