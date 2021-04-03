#include "lgraphic/PointLight.hpp"
#include <cmath>

PointLight::PointLight(Vec3 position, int width, int height, Vec3 color, float _near, float _far) : depthMap(width, height, _near, _far){
    this->position = position;
    this->color = color;
}

void PointLight::draw(Renderable **ptrArray, int count){
    depthMap.draw(position, ptrArray, count);
}

void PointLight::addTo(LShaderBuilder& s){
    LShader::EffectData data;
    s.newContext();
    std::string textureUniform = s.uniqueName("u_depthMap");
    std::string colorUniform = s.uniqueName("u_color");
    std::string lightPosUniform = s.uniqueName("u_lightPos");
    data.handles[0] = s.addFragmentUniform("samplerCubeShadow", textureUniform);
    data.textureSlot = s.nextTextureSlot();
    data.handles[1] = s.addFragmentUniform("vec3", colorUniform);
    data.handles[2] = s.addFragmentUniform("vec3", lightPosUniform);
    std::string depth = s.uniqueName("depth");
    std::string tdepth = s.uniqueName("tdepth");
    std::string direction = s.uniqueName("direction");
    std::string distance = s.uniqueName("distance");

    char buff[100];
    s.fMain << "vec3 " << direction << " = " << LShader::WORLD_POSITION << " - " << lightPosUniform << ";\n";

    std::string a = s.uniqueName("a");
    sprintf(buff, "%f", (depthMap.m_near + depthMap.m_far) / (depthMap.m_far - depthMap.m_near));
    s.fMain << "const float " << a << " = " << buff << ";\n";
    std::string b = s.uniqueName("b");
    sprintf(buff, "%f", 2.0f * depthMap.m_near * depthMap.m_far / (depthMap.m_far - depthMap.m_near));
    s.fMain << "const float " << b << " = " << buff << ";\n";
    s.fMain << "float " << depth << " = " << a << "-" << b << "/max(abs(" << direction << ".x), max(abs(" << direction << ".y), abs(" << direction << ".z)));\n";

    s.fMain << "float " << tdepth << " = texture(" << textureUniform << ",vec4(" << direction << ", (" << depth << "+1.0)/2.0*0.999));\n";
    s.fMain << "float " << distance << " = length(" << direction << ");\n";
    s.fMain << LShader::LIGHT_INTENSITY << " += " << tdepth << " * abs(dot(" << LShader::WORLD_NORMAL << ", " << direction << ")) * " << colorUniform << " / (" << distance << "*" << distance << "*" << distance << ");\n";
    s.addEffect(*this, data);
}

void PointLight::apply(LShader& s, const  LShader::EffectData& data){
    s.uniformCubeMap(data.handles[0], depthMap, data.textureSlot);
    s.uniformVec3(data.handles[1], color);
    s.uniformVec3(data.handles[2], position);
}
