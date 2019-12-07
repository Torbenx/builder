//#include "Python.h"
#include "BuildRenderer.hpp"
#include "util/log.hpp"
#include "graphic/Framebuffer.hpp"
#include "fullscreen.hpp"

BuildRenderer::BuildRenderer(){}

void BuildRenderer::create(){
    explorer.setPath("C:/");
}

void BuildRenderer::change(int newWidth, int newHeight){
    Framebuffer::DEFAULT.viewport(newWidth, newHeight);
    explorer.change(newWidth, newHeight);
}

void BuildRenderer::drawFrame(){
    Framebuffer::DEFAULT.bind();
    explorer.drawFrame();
}

void BuildRenderer::pointer(PointerEvent event){
    explorer.pointer(event);
}