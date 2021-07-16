#include "3d/PointLight.hpp"
#include "util/log.hpp"
#include <cmath>

PointLight::PointLight(Vec3 position, int width, int height, Vec3 color, float _near, float _far)
    : EffectInstance(EFFECT)
    , depthMap(CubeMap::depth(width, height, Texture::DepthBits::D16))
    , passes { &framebuffers[0], &framebuffers[1], &framebuffers[2], &framebuffers[3], &framebuffers[4], &framebuffers[5] }
    , position(position)
    , color(color)
    , m_near(_near)
    , m_far(_far)
    , width(width)
    , height(height) {

    updateViewProjectionMatrix();
}

void PointLight::Effect::addTo(EffectShaderBuilder& s) {
    s.beginEffect();
    std::string textureUniform = s.uniqueName("u_depthMap");
    std::string colorUniform = s.uniqueName("u_color");
    std::string lightPosUniform = s.uniqueName("u_lightPos");
    std::string factors = s.uniqueName("u_factors");
    s.addFragmentUniform("samplerCubeShadow", textureUniform);
    s.nextTextureSlot();
    s.addFragmentUniform("vec3", colorUniform);
    s.addFragmentUniform("vec3", lightPosUniform);
    s.addFragmentUniform("vec2", factors);
    std::string depth = s.uniqueName("depth");
    std::string result = s.uniqueName("result");
    std::string direction = s.uniqueName("direction");
    s.fMain << "vec3 " << direction << " = " << EffectShader::WORLD_POSITION << " - " << lightPosUniform << ";\n";
    s.fMain << "float " << depth << " = " << factors << ".x - " << factors << ".y / max(abs(" << direction << ".x), max(abs(" << direction << ".y), abs(" << direction << ".z)));\n";
    s.fMain << "float " << result << " = texture(" << textureUniform << ",vec4(" << direction << ", (" << depth << "+1.0)/2.0 - 0.001));\n";
    s.fMain << EffectShader::LIGHT_INTENSITY << " += " << result << " * abs(dot(" << EffectShader::WORLD_NORMAL << ", normalize(" << direction << "))) * " << colorUniform << " / dot(" << direction << "," << direction << ");\n";
}

void PointLight::applyTo(OrderedShader s) {
    s.uniformCubeMap(depthMap);
    s.uniformVec3(color);
    s.uniformVec3(position);
    s.uniformVec2(Vec2((m_near + m_far) / (m_far - m_near), 2.0f * m_near * m_far / (m_far - m_near)));
}

void PointLight::addObject(id_t id) {
    for (int_t i = 0; i < 6; i++) {
        passes[i].addObject(id);
    }
}

void PointLight::draw(GraphicsObjectCollection& objects) {
    for (int_t i = 0; i < 6; i++) {
        passes[i].draw(objects);
    }
}

void PointLight::updateViewProjectionMatrix() {
    Mat4x4 projection = Mat4x4::frustum(-m_near, m_near, -m_near, m_near, m_near, m_far);
    for (int_t i = 0; i < 6; i++) {
        framebuffers[i].bind();
        framebuffers[i].attach(depthMap, CubeMap::Faces[i]);
        framebuffers[i].viewport(width, height);

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
        passes[i].viewProjectionMatrix = projection * view;
    }
}
