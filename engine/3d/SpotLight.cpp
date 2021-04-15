#include "3d/SpotLight.hpp"
#include "util/log.hpp"

SpotLight::SpotLight(Vec3 position, Vec3 direction, float hfov, float vfov, Vec3 hDirection, int width, int height, Vec3 color, float _near, float _far)
    : EffectInstance(&EFFECT)
    , depthMap(Texture::depth(width, height, Texture::DepthBits::D16))
    , position(position)
    , direction(direction)
    , hfov(hfov)
    , vfov(vfov)
    , hDirection(hDirection)
    , color(color)
    , m_near(_near)
    , m_far(_far)
    , width(width)
    , height(height) {

    projection = Mat4x4::frustumVH(vfov, hfov, _near, _far);
    view = Mat4x4::look(position, direction, hDirection);
    framebuffer.bind();
    framebuffer.attach(depthMap);
    framebuffer.viewport(width, height);
}

SpotLight::SpotLight(Vec3 position, Vec3 direction, float fov, int texWidth, int texHeight, Vec3 color, float _near, float _far)
    : SpotLight(position, direction, fov, fov, Vec3::findOrthogonal(direction), texWidth, texHeight, color, _near, _far) { }

EffectBindings SpotLight::Effect::addTo(EffectShaderBuilder& s) {
    EffectBindings data;
    s.newContext();
    std::string mvpUniform = s.uniqueName("u_mvpMatrix");
    std::string textureUniform = s.uniqueName("u_depthMap");
    std::string colorUniform = s.uniqueName("u_color");
    std::string positionVarying = s.uniqueName("v_position");
    std::string lightPosUniform = s.uniqueName("u_lightPos");
    data.handles[0] = s.addVertexUniform("mat4", mvpUniform);
    data.handles[1] = s.addFragmentUniform("sampler2DShadow", textureUniform);
    data.textureSlot = s.nextTextureSlot();
    data.handles[2] = s.addFragmentUniform("vec3", colorUniform);
    s.addVarying("vec3", positionVarying);
    data.handles[3] = s.addFragmentUniform("vec3", lightPosUniform);
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
    return data;
}

void SpotLight::applyTo(EffectShader& s, const EffectBindings& data) {
    Mat4x4 mat = projection * view;
    s.uniformMat4x4(data.handles[0], mat);
    s.uniformTexture(data.handles[1], depthMap, data.textureSlot);
    s.uniformVec3(data.handles[2], color);
    s.uniformVec3(data.handles[3], position);
}

void SpotLight::addTo(Scene& scene) {
    scene.addEffect(this);
    scene.addDepthRenderpass(&framebuffer, projection * view);
}
