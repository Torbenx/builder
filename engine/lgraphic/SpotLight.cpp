#include "lgraphic/SpotLight.hpp"
#include "util/log.hpp"

SpotLight::SpotLight(Vec3 position, Vec3 direction, float hfov, float vfov, Vec3 hDirection, int texWidth, int texHeight, Vec3 color, float _near, float _far) :
    depthMap(texWidth, texHeight), position(position), direction(direction), hfov(hfov), vfov(vfov), hDirection(hDirection), color(color), m_near(_near), m_far(_far)
{
    projection = Mat4x4::frustumVH(vfov, hfov, _near, _far);
    view = Mat4x4::look(position, direction, hDirection);
}

SpotLight::SpotLight(Vec3 position, Vec3 direction, float fov, int texWidth, int texHeight, Vec3 color, float _near, float _far)
    : SpotLight(position, direction, fov, fov, Vec3::findOrthogonal(direction), texWidth, texHeight, color, _near, _far) {

}

void SpotLight::draw(Renderable **ptrArray,int count){
    Mat4x4 mat = projection * view;
    depthMap.draw(mat, ptrArray, count);
}

void SpotLight::addTo(LShaderBuilder& s){
    LShader::EffectData data;
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
    s.addVarying("vec4", positionVarying);
    data.handles[3] = s.addFragmentUniform("vec3", lightPosUniform);
    std::string depth = s.uniqueName("depth");
    std::string direction = s.uniqueName("direction");
    std::string distance = s.uniqueName("distance");
    std::string pos = s.uniqueName("pos");
    s.vMain << positionVarying << " = " << mvpUniform << " * vec4(" << LShader::WORLD_POSITION << ",1.0);\n";
    s.fMain << "vec3 " << pos << " = " << positionVarying << ".xyz/" << positionVarying << ".w;\n";
    s.fMain << "vec3 " << direction << " = " << LShader::WORLD_POSITION << " - " << lightPosUniform << ";\n";
    s.fMain << "float " << distance << " = length(" << direction << ");\n";
    s.fMain << direction << " /= " << distance << ";\n";
    s.fMain << "float " << depth << " = texture(" << textureUniform << ",(" << pos << "+1.0)/2.0 * vec3(1.0, 1.0, 0.999));\n";
    s.fMain << "if(" << "dot(" << pos << ".xy," << pos << ".xy) <= 1.0 && " << pos << ".z >= 0.0){\n";
    s.fMain << LShader::LIGHT_INTENSITY << " += " << depth << " * max(0.0, -dot(" << LShader::WORLD_NORMAL << ", " << direction << ")) * " << colorUniform << " / (" << distance << "*" << distance << ");\n}\n";
    s.addEffect(*this, data);
}

void SpotLight::apply(LShader& s, const LShader::EffectData& data){
    Mat4x4 mat = projection * view;
    s.uniformMat4x4(data.handles[0], mat);
    s.uniformTexture(data.handles[1], depthMap, data.textureSlot);
    s.uniformVec3(data.handles[2], color);
    s.uniformVec3(data.handles[3], position);
}
