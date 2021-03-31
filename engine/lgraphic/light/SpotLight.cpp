#include "SpotLight.hpp"
#include "../../util/log.hpp"

SpotLight::SpotLight(Vec3 position,Vec3 direction,float hfov,float vfov,Vec3 hDirection,int texWidth,int texHeight,Vec3 color,float _near,float _far) : depthMap(texWidth,texHeight,_far){
    this->position = position;
    this->direction = direction;
    this->hDirection = hDirection;
    this->hfov = hfov;
    this->vfov = vfov;
    width = texWidth;
    height = texHeight;
    this->color = color;
    mvpMatrix.projection = Mat4x4::frustumVH(vfov,hfov,_near,_far);
}

SpotLight::SpotLight(Vec3 position, Vec3 direction, float fov, int texWidth, int texHeight, Vec3 color, float _near, float _far)
    : SpotLight(position, direction, fov, fov, Vec3::findOrthogonal(direction), texWidth, texHeight, color, _near, _far) {

}

SpotLight::~SpotLight(){

}

void SpotLight::init(){
    depthMap.init();
}

void SpotLight::draw(Renderable **ptrArray,int count){
    mvpMatrix.model = Mat4x4::Id;
    mvpMatrix.view = Mat4x4::look(position, direction, hDirection);
    depthMap.draw(mvpMatrix, position, ptrArray, count);
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
    data.handles[1] = s.addFragmentUniform("sampler2D", textureUniform);
    data.textureSlot = s.nextTextureSlot();
    data.handles[2] = s.addFragmentUniform("vec3", colorUniform);
    s.addVarying("vec4", positionVarying);
    data.handles[3] = s.addFragmentUniform("vec3", lightPosUniform);
    std::string depth = s.uniqueName("depth");
    std::string direction = s.uniqueName("direction");
    std::string distance = s.uniqueName("distance");
    std::string m_far = s.uniqueName("far");
    std::string pos = s.uniqueName("pos");
    s.vMain << positionVarying << " = " << mvpUniform << " * vec4(" << LShader::WORLD_POSITION << ",1.0);\n";
    char buff[100];
    sprintf(buff, "%f", depthMap.m_far);
    s.fMain << "const float " << m_far << " = " << buff << ";\n";
    s.fMain << "vec2 " << pos << " = " << positionVarying << ".xy/" << positionVarying << ".w;\n";
    s.fMain << "vec3 " << direction << " = " << LShader::WORLD_POSITION << " - " << lightPosUniform << ";\n";
    s.fMain << "float " << distance << " = length(" << direction << ");\n";
    s.fMain << direction << " /= " << distance << ";\n";
    s.fMain << "float " << depth << " = dot(vec4(" << m_far << "," << m_far << "/255.0," << m_far << "/255.0/255.0," << m_far << "/255.0/255.0/255.0),texture2D(" << textureUniform << ",(" << pos << "+1.0)/2.0));\n";
    s.fMain << "if(" << positionVarying << ".z>0.0&&dot(" << pos << "," << pos << ")<=1.0&&" << distance << "<=" << depth << "*1.01){\n";
    s.fMain << LShader::LIGHT_INTENSITY << " += max(0.0, -dot(" << LShader::WORLD_NORMAL << ", " << direction << ")) * " << colorUniform << " / (" << distance << "*" << distance << ");\n}\n";
    s.addEffect(*this, data);
}

void SpotLight::apply(LShader& s, const LShader::EffectData& data){
    s.uniformMat4x4(data.handles[0], mvpMatrix.setupMvp());
    s.uniformTexture(data.handles[1], *depthMap.texture, data.textureSlot);
    s.uniformVec3(data.handles[2], color);
    s.uniformVec3(data.handles[3], position);
}
