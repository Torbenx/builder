#include <stdio.h>
#include "DepthMap.hpp"

DepthMap::DepthMap(int width,int height,float _far){
    shader = Shaders::depth();
    this->width = width;
    this->height = height;
    this->m_far = _far;
}

DepthMap::~DepthMap(){

}

void DepthMap::init(){
    texture.init();
    renderbuffer.init();
    framebuffer.init();
    framebuffer.colorAttachment(texture);
    renderbuffer.storageDepth16(width,height);
    framebuffer.depthAttachment(renderbuffer);
    texture.texture(NULL,width,height,GL_RGBA,GL_UNSIGNED_BYTE);
    framebuffer.viewport(width,height);
    framebuffer.setClearColor(1,1,1,1);
}

void DepthMap::draw(MvpMatrix& mvpMatrix, Vec3 position, Renderable **ptrArray, int count){
    framebuffer.bind();
    shader->use();
    shader->uniformMat4x4(shader->mvpUniform, mvpMatrix.setupMvp());
    shader->uniformVec3(shader->lightPosUniform, position);
    shader->uniform1f(shader->farUniform, m_far);
    glEnable(GL_DEPTH_TEST);
    int i;
    for(i=0;i<count;i++){
        ptrArray[i]->draw(*shader, shader->positionAttrib, shader->modelUniform);
    }
}
