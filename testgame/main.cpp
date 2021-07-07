#include "TestRenderer.hpp"
#include <main.hpp>

template<Gapi G>
Renderer<G>* createRenderer() {
    return new TestRenderer<G>();
}

INSTANTIATE_CREATE_RENDERER_FUNCTION();
