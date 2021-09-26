#include "3d/PointLight.hpp"
#include "util/log.hpp"
#include <cmath>

PointLight::PointLight() { }

void PointLight::addTo(EffectShaderBuilder& s) {
    std::string textureUniform = s.uniqueName("u_depthMap");
    std::string colorUniform = s.uniqueName("u_color");
    std::string lightPosUniform = s.uniqueName("u_lightPos");
    std::string a = s.uniqueName("u_a");
    std::string b = s.uniqueName("u_b");
    s.addTextureUniform("samplerCubeShadow", textureUniform);

    s.beginVertexUniformBuffer(s.uniqueName("PointlightVert"));
    s.vInit << "vec3 " << lightPosUniform << ";\n";
    s.endVertexUniformBuffer();

    s.beginFragmentUniformBuffer(s.uniqueName("PointLightFrag"));
    s.fInit << "vec3 " << colorUniform << ";\n";
    s.fInit << "float " << a << ";\n";
    s.fInit << "float " << b << ";\n";
    s.endFragmentUniformBuffer();

    std::string depth = s.uniqueName("depth");
    std::string result = s.uniqueName("result");
    std::string direction = s.uniqueName("v_direction");
    std::string cosTheta = s.uniqueName("cosTheta");
    s.addVarying("vec3", direction);
    s.vMain << direction << " = " << EffectShader::WORLD_POSITION << " - " << lightPosUniform << ";\n";
    s.fMain << "float " << cosTheta << " = max(-dot(" << EffectShader::WORLD_NORMAL << ", normalize(" << direction << ")), 0.0);\n";
    s.fMain << "float " << depth << " = " << a << " - " << b << " / max(abs(" << direction << ".x), max(abs(" << direction << ".y), abs(" << direction << ".z)));\n";
    s.fMain << "float " << result << " = texture(" << textureUniform << ",vec4(" << direction << ", " << depth << "));\n";
    s.fMain << EffectShader::LIGHT_INTENSITY << " += " << result << " * " << cosTheta << " * " << colorUniform << " / dot(" << direction << "," << direction << ");\n";
}
