#include "lgraphic/PointLight.hpp"
#include <cmath>

PointLight::PointLight(Vec3 position, int width, int height, Vec3 color, float _near, float _far)
    : depthMap(width, height, _near, _far), position(position), color(color) { }

void PointLight::draw(void (*doDraw)(Framebuffer& framebuffer, Shader& shader, Shader::LocationHandle positionAttrib, Shader::LocationHandle modelUniform)) {
    depthMap.draw(position, doDraw);
}

LShader::EffectLocationHandle PointLight::addTo(LShaderBuilder& s) {
    LShader::EffectData data;
    s.newContext();
    std::string textureUniform = s.uniqueName("u_depthMap");
    std::string colorUniform = s.uniqueName("u_color");
    std::string lightPosUniform = s.uniqueName("u_lightPos");
    std::string factors = s.uniqueName("u_factors");
    data.handles[0] = s.addFragmentUniform("samplerCubeShadow", textureUniform);
    data.textureSlot = s.nextTextureSlot();
    data.handles[1] = s.addFragmentUniform("vec3", colorUniform);
    data.handles[2] = s.addFragmentUniform("vec3", lightPosUniform);
    data.handles[3] = s.addFragmentUniform("vec2", factors);
    std::string depth = s.uniqueName("depth");
    std::string result = s.uniqueName("result");
    std::string direction = s.uniqueName("direction");
    s.fMain << "vec3 " << direction << " = " << LShader::WORLD_POSITION << " - " << lightPosUniform << ";\n";
    s.fMain << "float " << depth << " = " << factors << ".x - " << factors << ".y / max(abs(" << direction << ".x), max(abs(" << direction << ".y), abs(" << direction << ".z)));\n";
    s.fMain << "float " << result << " = texture(" << textureUniform << ",vec4(" << direction << ", (" << depth << "+1.0)/2.0*0.99));\n";
    s.fMain << LShader::LIGHT_INTENSITY << " += " << result << " * abs(dot(" << LShader::WORLD_NORMAL << ", normalize(" << direction << "))) * " << colorUniform << " / dot(" << direction << "," << direction << ");\n";
    return s.addEffect(data);
}

void PointLight::bindTo(LShader& s, const LShader::EffectData& data) {
    s.uniformCubeMap(data.handles[0], depthMap, data.textureSlot);
    s.uniformVec3(data.handles[1], color);
    s.uniformVec3(data.handles[2], position);
    s.uniformVec2(data.handles[3], Vec2((depthMap.m_near + depthMap.m_far) / (depthMap.m_far - depthMap.m_near), 2.0f * depthMap.m_near * depthMap.m_far / (depthMap.m_far - depthMap.m_near)));
}
