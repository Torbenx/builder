#include "lgraphic/DepthMap.hpp"
#include <stdio.h>

DepthMap::DepthMap(int width, int height) :
    Texture(Texture::depth(width, height, Texture::DepthBits::D16).move()),
    shader(Shaders::depth())
{
    framebuffer.attach(*this);
    framebuffer.viewport(width, height);
}

void DepthMap::draw(Mat4x4& vpMatrix, Renderable **ptrArray, int count){
    GL_VERIFY(glIsEnabled(GL_DEPTH_TEST));
    framebuffer.bind();
    framebuffer.clear();
    shader->use();
    shader->uniformMat4x4(shader->mvpUniform, vpMatrix);
    for(int i=0;i<count;i++){
        ptrArray[i]->draw(framebuffer, *shader, shader->positionAttrib, shader->modelUniform);
    }
}
