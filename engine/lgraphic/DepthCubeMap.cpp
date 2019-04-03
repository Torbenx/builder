#include "DepthCubeMap.hpp"
#define _USE_MATH_DEFINES
#include <cmath>
#include <stdio.h>

DepthCubeMap::DepthCubeMap(int width,int height,float _far){
    this->width = width;
    this->height = height;
    this->m_far = _far;
    vSource = 
        "attribute vec4 a_position;\n"
        "uniform mat4 u_mvpMatrix;\n"
        "uniform mat4 u_modelMatrix;\n"
        "varying vec4 v_position;\n"
        "void main(){\n"
        "    v_position = u_modelMatrix*a_position;\n"
        "    gl_Position = u_mvpMatrix*v_position;\n"
        //"    gl_Position = vec4(gl_Position.xy / gl_Position.z, gl_Position.z, 1.0);\n"
        "}";
    std::string f = 
        "varying vec4 v_position;\n"
        "uniform vec3 u_lightPos;\n"
        "void main(){\n"
        "    float f0 = distance(u_lightPos,v_position.xyz)*255.0/";
    char buff[100];
    sprintf(buff,"%f",m_far);
    f.append(buff);
    //f.append("/v_position.w");
    f.append(
        "    ;\n"
        "    float f1 = fract(f0);\n"
        "    f0 -= f1;\n"
        "    f1 *= 255.0;\n"
        "    float f2 = fract(f1);\n"
        "    f1 -= f2;\n"
        "    f2 *= 255.0;\n"
        "    float f3 = fract(f2);\n"
        "    f2 -= f3;\n"
        "    f3 *= 255.0;\n"
        "    f3 = ceil(f3);\n"
        "    gl_FragColor = vec4(f0,f1,f2,f3)/255.0;\n"
        //"    gl_FragColor = vec4(0,0,0,1);\n"
        "}");
    fSource = f;
    shader = new Shader(vSource,fSource);
}
        
DepthCubeMap::~DepthCubeMap(){
    delete shader;
}
        
void DepthCubeMap::init(){
    texture.init();
    renderbuffer.init();
    framebuffer.init();
    shader->init();
    texture.texturePosX(NULL,width,height,GL_RGBA,GL_UNSIGNED_BYTE);
    texture.textureNegX(NULL,width,height,GL_RGBA,GL_UNSIGNED_BYTE);
    texture.texturePosY(NULL,width,height,GL_RGBA,GL_UNSIGNED_BYTE);
    texture.textureNegY(NULL,width,height,GL_RGBA,GL_UNSIGNED_BYTE);
    texture.texturePosZ(NULL,width,height,GL_RGBA,GL_UNSIGNED_BYTE);
    texture.textureNegZ(NULL,width,height,GL_RGBA,GL_UNSIGNED_BYTE);
    renderbuffer.storageDepth16(width,height);
    framebuffer.depthAttachment(renderbuffer);
    framebuffer.setClearColor(1,1,1,1);
    framebuffer.viewport(width,height);
}
        
void DepthCubeMap::draw(Mat4x4 model, Vec3 position, Renderable **ptrArray, int count){
    shader->use();
    MvpMatrix mvpMatrix;
    float *m;
    //mvpMatrix.projection.setFrustum(-1, 1, -1, 1, 1, 10);
    mvpMatrix.projection = Mat4x4::Id;
    m = mvpMatrix.projection.matrix;
    m[4+4+4+3] = 0;
    m[4+4+3] = -1;
    m[4+4+2] = -1;
    m[4+4+4+2] = -0.1;
    mvpMatrix.model = model;
    GLboolean depthTest = glIsEnabled(GL_DEPTH_TEST);
    glEnable(GL_DEPTH_TEST);
    shader->uniformVec3("u_lightPos",position);
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
        shader->uniformMat4x4("u_mvpMatrix",mvpMatrix.setupMvp());
        //mvpMatrix.view.debug();
        framebuffer.colorAttachment(targets[face],texture);
        framebuffer.bind();
        //if(face != 2 && face != 3)continue;
        
        for(i=0;i<count;i++){
            ptrArray[i]->draw(shader,"a_position","u_modelMatrix");
        }
    }
    Framebuffer::DEFAULT.bind();
    if(!depthTest)glDisable(GL_DEPTH_TEST);
}
