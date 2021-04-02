#include "lgraphic/PointLight.hpp"
#include <cmath>

PointLight::PointLight(Vec3 position,int width,int height,Vec3 color,float near,float _far) : depthMap(width,height,_far){
    this->position = position;
    this->color = color;
}

PointLight::~PointLight(){

}

void PointLight::draw(Renderable **ptrArray, int count){
    depthMap.draw(Mat4x4::Id, position, ptrArray, count);
}

void PointLight::addTo(LShaderBuilder& s){
    LShader::EffectData data;
    s.newContext();
    std::string textureUniform = s.uniqueName("u_depthMap");
    std::string colorUniform = s.uniqueName("u_color");
    std::string lightPosUniform = s.uniqueName("u_lightPos");
    data.handles[0] = s.addFragmentUniform("samplerCube", textureUniform);
    data.textureSlot = s.nextTextureSlot();
    data.handles[1] = s.addFragmentUniform("vec3", colorUniform);
    data.handles[2] = s.addFragmentUniform("vec3", lightPosUniform);
    std::string _far = s.uniqueName("far");
    std::string depth = s.uniqueName("depth");
    std::string direction = s.uniqueName("direction");
    std::string distance = s.uniqueName("distance");

    char buff[100];
    sprintf(buff, "%f", depthMap.m_far);
    s.fMain << "const float " << _far << " = " << buff << ";\n";

    s.fMain << "vec3 " << direction << " = " << LShader::WORLD_POSITION << " - " << lightPosUniform << ";\n";
    s.fMain << "float " << depth << " = dot(vec4(" << _far << "," << _far << "/255.0," << _far << "/255.0/255.0," << _far << "/255.0/255.0/255.0),textureCube(" << textureUniform << "," << direction << "));\n";
    s.fMain << "float " << distance << " = length(" << direction << ");\n";
    s.fMain << direction << " /= " << distance << ";\n";
    s.fMain << "if(" << distance << "<=" << depth << "*1.01){\n";
    s.fMain << LShader::LIGHT_INTENSITY << " += abs(dot(" << LShader::WORLD_NORMAL << ", " << direction << ")) * " << colorUniform << " / (" << distance << "*" << distance << ");\n}\n";
    s.addEffect(*this, data);
}

void PointLight::apply(LShader& s, const  LShader::EffectData& data){
    s.uniformCubeMap(data.handles[0], depthMap.texture, data.textureSlot);
    s.uniformVec3(data.handles[1], color);
    s.uniformVec3(data.handles[2], position);
}
