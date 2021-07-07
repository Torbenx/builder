#include "BuildRenderer.hpp"
#include <main.hpp>

template<Gapi G>
Renderer<G>* createRenderer(){
    return new BuildRenderer<G>();
}

INSTANTIATE_CREATE_RENDERER_FUNCTION();
