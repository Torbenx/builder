#pragma once

#include <Renderer.hpp>

template<Gapi G>
Renderer<G> *createRenderer();

#define __FOREACH_G_API_INSTANTIATE_CREATE_RENDERER_FUNCTION(api) template Renderer<api>* createRenderer<api>();
#define INSTANTIATE_CREATE_RENDERER_FUNCTION() ENUMERATE_G_APIS(__FOREACH_G_API_INSTANTIATE_CREATE_RENDERER_FUNCTION)
