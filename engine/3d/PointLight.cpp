#include "3d/PointLight.hpp"
#include <cmath>

PointLight::PointLight(Vec3 position, int width, int height, Vec3 color, float _near, float _far)
    : EffectInstance(&EFFECT)
    , m_near(_near)
    , m_far(_far)
    , position(position)
    , color(color)
    , depthMap(CubeMap::depth(width, height, Texture::DepthBits::D16))
    , width(width)
    , height(height) {

    for (int i = 0; i < 6; i++) {
        framebuffers[i].bind();
        framebuffers[i].attach(depthMap, CubeMap::Faces[i]);
        framebuffers[i].viewport(width, height);
    }
}

EffectBindings PointLight::Effect::addTo(EffectShaderBuilder& s) {
    EffectBindings data;
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
    s.fMain << "vec3 " << direction << " = " << EffectShader::WORLD_POSITION << " - " << lightPosUniform << ";\n";
    s.fMain << "float " << depth << " = " << factors << ".x - " << factors << ".y / max(abs(" << direction << ".x), max(abs(" << direction << ".y), abs(" << direction << ".z)));\n";
    s.fMain << "float " << result << " = texture(" << textureUniform << ",vec4(" << direction << ", (" << depth << "+1.0)/2.0 - 0.01));\n";
    s.fMain << EffectShader::LIGHT_INTENSITY << " += " << result << " * abs(dot(" << EffectShader::WORLD_NORMAL << ", normalize(" << direction << "))) * " << colorUniform << " / dot(" << direction << "," << direction << ");\n";
    return data;
}

void PointLight::applyTo(EffectShader& s, const EffectBindings& data) {
    s.uniformCubeMap(data.handles[0], depthMap, data.textureSlot);
    s.uniformVec3(data.handles[1], color);
    s.uniformVec3(data.handles[2], position);
    s.uniformVec2(data.handles[3], Vec2((m_near + m_far) / (m_far - m_near), 2.0f * m_near * m_far / (m_far - m_near)));
}

void PointLight::addTo(Scene& scene) {
    Mat4x4 projection = Mat4x4::frustum(-m_near, m_near, -m_near, m_near, m_near, m_far);
    for (int i = 0; i < 6; i++) {
        Mat4x4 view;
        switch (CubeMap::Faces[i]) {
        case CubeMap::PosX:
            view = Mat4x4::look(position, Vec3(+1, 0, 0), Vec3(0, -1, 0));
            break;
        case CubeMap::NegX:
            view = Mat4x4::look(position, Vec3(-1, 0, 0), Vec3(0, -1, 0));
            break;
        case CubeMap::PosY:
            view = Mat4x4::look(position, Vec3(0, +1, 0), Vec3(0, 0, +1));
            break;
        case CubeMap::NegY:
            view = Mat4x4::look(position, Vec3(0, -1, 0), Vec3(0, 0, -1));
            break;
        case CubeMap::PosZ:
            view = Mat4x4::look(position, Vec3(0, 0, +1), Vec3(0, -1, 0));
            break;
        case CubeMap::NegZ:
            view = Mat4x4::look(position, Vec3(0, 0, -1), Vec3(0, -1, 0));
            break;
        }
        scene.addDepthRenderpass(&framebuffers[i], projection * view);
    }
    scene.addEffect(this);
}
