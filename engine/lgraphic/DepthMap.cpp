#include <stdio.h>
#include "DepthMap.hpp"

std::unique_ptr<DepthMap::DepthShader> DepthMap::DepthShader::construct(float _far){
    auto s = std::make_unique<DepthMap::DepthShader>();
    ShaderBuilder b;
    s->positionHandle = b.addAttrib("vec4", "a_position");
    s->mvpMatrixHandle = b.addVertexUniform("mat4", "u_mvpMatrix");
    s->modelMatrixHandle = b.addVertexUniform("mat4", "u_modelMatrix");
    s->lightPosHandle = b.addFragmentUniform("vec3", "u_lightPos");
    b.addVarying("vec3", "v_position");
    b.vMain << "    vec4 pos = u_modelMatrix * a_position;\n";
    b.vMain << "    v_position = pos.xyz;\n";
    b.vMain << "    gl_Position = u_mvpMatrix * pos;\n";
    b.fMain << "    float f0 = distance(u_lightPos, v_position)*255.0/";
    char buff[100];
    sprintf(buff, "%f", _far);
    b.fMain << buff << ";\n";
    b.fMain << "    float f1 = fract(f0);\n";
    b.fMain << "    f0 -= f1;\n";
    b.fMain << "    f1 *= 255.0;\n";
    b.fMain << "    float f2 = fract(f1);\n";
    b.fMain << "    f1 -= f2;\n";
    b.fMain << "    f2 *= 255.0;\n";
    b.fMain << "    float f3 = fract(f2);\n";
    b.fMain << "    f2 -= f3;\n";
    b.fMain << "    f3 *= 255.0;\n";
    b.fMain << "    f3 = ceil(f3);\n";
    b.fMain << "    gl_FragColor = vec4(f0,f1,f2,f3)/255.0;\n";
    s->initalize(b);
    return s;
}

DepthMap::DepthMap(int width,int height,float _far){
    shader = DepthShader::construct(_far);
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
    shader->uniformMat4x4(shader->mvpMatrixHandle, mvpMatrix.setupMvp());
    shader->uniformVec3(shader->lightPosHandle, position);
    GLboolean depthTest = glIsEnabled(GL_DEPTH_TEST);
    glEnable(GL_DEPTH_TEST);
    int i;
    for(i=0;i<count;i++){
        ptrArray[i]->draw(*shader, shader->positionHandle, shader->modelMatrixHandle);
    }
    Framebuffer::DEFAULT.bind();
    if(!depthTest)glDisable(GL_DEPTH_TEST);
}
