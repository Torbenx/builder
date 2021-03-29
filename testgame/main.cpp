#include "TestRenderer.hpp"
#include <main.hpp>

Renderer *createRenderer(){
    return new TestRenderer();
}
