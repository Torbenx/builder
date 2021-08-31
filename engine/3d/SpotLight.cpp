#include "3d/SpotLight.hpp"
#include "util/log.hpp"

template<>
const Effect::Type EffectImpl<SpotLight>::effectType = registerEffectType();

SpotLight::SpotLight(Vec3 position, Vec3 direction, float hfov, float vfov, Vec3 hDirection, int width, int height, Vec3 color, float _near, float _far)
    : DepthRenderpass(&framebuffer)
    , depthMap(Texture::depth(width, height, Texture::DepthBits::D16))
    , color(color)
    , direction(direction)
    , position(position)
    , hDirection(hDirection)
    , hfov(hfov)
    , vfov(vfov)
    , m_near(_near)
    , m_far(_far)
    , width(width)
    , height(height) {

    projection = Mat4x4::frustumVH(this->vfov, this->hfov, m_near, m_far);
    view = Mat4x4::look(position, direction, hDirection);
    viewProjectionMatrix = projection * view;
    framebuffer.bind();
    framebuffer.attach(depthMap);
    framebuffer.viewport(this->width, this->height);
}

SpotLight::SpotLight(Vec3 position, Vec3 direction, float fov, int texWidth, int texHeight, Vec3 color, float _near, float _far)
    : SpotLight(position, direction, fov, fov, Vec3::findOrthogonal(direction), texWidth, texHeight, color, _near, _far) { }

void SpotLight::addTo(EffectShaderBuilder& s) {
    std::string mvpUniform = s.uniqueName("u_mvpMatrix");
    std::string textureUniform = s.uniqueName("u_depthMap");
    std::string colorUniform = s.uniqueName("u_color");
    std::string positionVarying = s.uniqueName("v_position");
    std::string lightPosUniform = s.uniqueName("u_lightPos");
    s.addFragmentUniform("sampler2DShadow", textureUniform);
    s.nextTextureSlot();
    s.addVertexUniform("mat4", mvpUniform);
    s.addFragmentUniform("vec3", colorUniform);
    s.addFragmentUniform("vec3", lightPosUniform);
    s.addVarying("vec3", positionVarying);
    std::string result = s.uniqueName("result");
    std::string direction = s.uniqueName("direction");
    std::string pos = s.uniqueName("pos");
    s.vMain << "vec4 " << pos << " = " << mvpUniform << " * vec4(" << EffectShader::WORLD_POSITION << ",1.0);\n";
    s.vMain << positionVarying << " = " << pos << ".xyz / " << pos << ".w;\n";
    s.fMain << "vec3 " << direction << " = " << EffectShader::WORLD_POSITION << " - " << lightPosUniform << ";\n";
    s.fMain << "float " << result << " = texture(" << textureUniform << ",(" << positionVarying << " + 1.0) / 2.0 - vec3(0.0, 0.0, 0.001));\n";
    s.fMain << "if("
            << "dot(" << positionVarying << ".xy," << positionVarying << ".xy) <= 1.0 && " << positionVarying << ".z >= -1.0){\n";
    s.fMain << EffectShader::LIGHT_INTENSITY << " += " << result << " * max(0.0, -dot(" << EffectShader::WORLD_NORMAL << ", normalize(" << direction << "))) * " << colorUniform << " / dot(" << direction << "," << direction << ");\n}\n";
}

void SpotLight::applyTo(OrderedShader s) {
    s.uniformMat4x4(viewProjectionMatrix);
    s.uniformVec3(color);
    s.uniformVec3(position);
}

SpotLight::~SpotLight() {
    depthMap.release();
}
