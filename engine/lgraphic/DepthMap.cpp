#include <stdio.h>
#include "DepthMap.hpp"

DepthMap::DepthMap(int width,int height,float _far) :
    Texture(Texture::color(width, height, Texture::Format::RGBA8888).move()),
    depthAttachment(Texture::depth(width, height, 16, true).move())
{
    shader = Shaders::depth();
    this->m_far = _far;
    framebuffer.colorAttachment(*this);
    framebuffer.depthAttachment(depthAttachment);
    framebuffer.viewport(width, height);
    framebuffer.clearColor(1, 1, 1, 1);
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
    shader->uniform1f(shader->farUniform, m_far);
    int i;
    for(i=0;i<count;i++){
        ptrArray[i]->draw(*shader, shader->positionAttrib, shader->modelUniform);
    }
}
