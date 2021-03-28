#include <stdio.h>
#include "DepthMap.hpp"

DepthMap::DepthShader::DepthShader(float _far){
    positionHandle = addAttrib("vec4", "a_position");
    mvpMatrixHandle = addVertexUniform("mat4", "u_mvpMatrix");
    modelMatrixHandle = addVertexUniform("mat4", "u_modelMatrix");
    lightPosHandle = addFragmentUniform("vec3", "u_lightPos");
    addVarying("vec3", "v_position");
    vMain <<
    "    vec4 pos = u_modelMatrix * a_position;\n"
    "    v_position = pos.xyz;\n"
    "    gl_Position = u_mvpMatrix * pos;\n";

    fMain <<
    "    float f0 = distance(u_lightPos, v_position)*255.0/";
    char buff[100];
    sprintf(buff, "%f", _far);
    fMain << buff << ";\n" <<
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
    "    gl_FragColor = vec4(f0,f1,f2,f3)/255.0;\n";
}

DepthMap::DepthMap(int width,int height,float _far) : shader(_far){
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
    shader.init();
    framebuffer.colorAttachment(texture);
    renderbuffer.storageDepth16(width,height);
    framebuffer.depthAttachment(renderbuffer);
    texture.texture(NULL,width,height,GL_RGBA,GL_UNSIGNED_BYTE);
    framebuffer.viewport(width,height);
    framebuffer.setClearColor(1,1,1,1);
}

void DepthMap::draw(MvpMatrix& mvpMatrix, Vec3 position, Renderable **ptrArray, int count){
    framebuffer.bind();
    shader.use();
    shader.uniformMat4x4(shader.mvpMatrixHandle, mvpMatrix.setupMvp());
    shader.uniformVec3(shader.lightPosHandle, position);
    GLboolean depthTest = glIsEnabled(GL_DEPTH_TEST);
    glEnable(GL_DEPTH_TEST);
    int i;
    for(i=0;i<count;i++){
        ptrArray[i]->draw(shader, shader.positionHandle, shader.modelMatrixHandle);
    }
    Framebuffer::DEFAULT.bind();
    if(!depthTest)glDisable(GL_DEPTH_TEST);
}
