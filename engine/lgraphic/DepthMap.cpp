#include "lgraphic/DepthMap.hpp"
#include <stdio.h>

DepthMap::DepthMap(int width, int height) :
    Texture(Texture::color(width, height, Texture::Format::RGBA8888).move()),
    depthAttachment(Texture::depth(width, height, 16).move())
{
    shader = Shaders::depth();
    //framebuffer.colorAttachment(*this);
    framebuffer.depthAttachment(depthAttachment);
    framebuffer.viewport(width, height);
    //framebuffer.clearColor(1, 1, 1, 1);
}

DepthMap::~DepthMap(){

}

void DepthMap::draw(MvpMatrix& mvpMatrix, Vec3 position, Renderable **ptrArray, int count){
    GL_VERIFY(glIsEnabled(GL_DEPTH_TEST));
    framebuffer.bind();
    framebuffer.clear();
    shader->use();
    shader->uniformMat4x4(shader->mvpUniform, mvpMatrix.setupMvp());
    shader->uniformVec3(shader->lightPosUniform, position);
    shader->uniform1f(shader->farUniform, 100.0f);
    int i;
    for(i=0;i<count;i++){
        ptrArray[i]->draw(framebuffer, *shader, shader->positionAttrib, shader->modelUniform);
    }
}
