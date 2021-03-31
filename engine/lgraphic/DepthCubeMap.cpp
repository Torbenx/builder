#include "DepthCubeMap.hpp"

DepthCubeMap::DepthCubeMap(int width,int height,float _far){
    shader = Shaders::depth();
    this->width = width;
    this->height = height;
    this->m_far = _far;
}

DepthCubeMap::~DepthCubeMap(){

}

void DepthCubeMap::init(){
    texture.init();
    texture.texturePosX(NULL, width, height, GL_RGBA, GL_UNSIGNED_BYTE);
    texture.textureNegX(NULL, width, height, GL_RGBA, GL_UNSIGNED_BYTE);
    texture.texturePosY(NULL, width, height, GL_RGBA, GL_UNSIGNED_BYTE);
    texture.textureNegY(NULL, width, height, GL_RGBA, GL_UNSIGNED_BYTE);
    texture.texturePosZ(NULL, width, height, GL_RGBA, GL_UNSIGNED_BYTE);
    texture.textureNegZ(NULL, width, height, GL_RGBA, GL_UNSIGNED_BYTE);
    renderbuffer.storageDepth16(width, height);
    framebuffer.depthAttachment(renderbuffer);
    framebuffer.clearColor(1, 1, 1, 1);
    framebuffer.viewport(width,height);
}

void DepthCubeMap::draw(Mat4x4 model, Vec3 position, Renderable **ptrArray, int count){
    GL_VERIFY(glIsEnabled(GL_DEPTH_TEST));
    framebuffer.bind();
    shader->use();
    MvpMatrix mvpMatrix;
    float *m;
    mvpMatrix.projection = Mat4x4::frustum(-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 100.0f);
    mvpMatrix.model = model;
    shader->uniformVec3(shader->lightPosUniform, position);
    shader->uniform1f(shader->farUniform, m_far);
    int face,i;
    for(face=0;face<6;face++){
        mvpMatrix.view = Mat4x4::Id;
        m = mvpMatrix.view.matrix;
        if(face==0){
            //dir = Vec3(1,0,0);
            m[4+1] = -1;
            m[0] = 0;
            m[4+4+2] = 0;
            m[4+4] = -1;
            m[2] = -1;
        }else if(face==1){
            //dir = Vec3(-1,0,0);
            m[4+1] = -1;
            m[0] = 0;
            m[4+4+2] = 0;
            m[4+4] = 1;
            m[2] = 1;
        }else if(face==2){
            //dir = Vec3(0,1,0);
            m[4+1] = 0;
            m[4+4+2] = 0;
            m[4+4+1] = 1;
            m[4+2] = -1;
        }else if(face==3){
            //dir = Vec3(0,-1,0);
            m[4+1] = 0;
            m[4+4+2] = 0;
            m[4+4+1] = -1;
            m[4+2] = 1;
        }else if(face==4){
            //dir = Vec3(0,0,1);
            m[4+1] = -1;
            m[4+4+2] = -1;
        }else if(face==5){
            //dir = Vec3(0,0,-1);
            m[0] = -1;
            m[4+1] = -1;
        }
        mvpMatrix.view *= Mat4x4::translate(position*-1);
        shader->uniformMat4x4(shader->mvpUniform, mvpMatrix.setupMvp());
        framebuffer.colorAttachment(targets[face], texture);
        framebuffer.clear();
        for(i=0;i<count;i++){
            ptrArray[i]->draw(*shader, shader->positionAttrib, shader->modelUniform);
        }
    }
}
