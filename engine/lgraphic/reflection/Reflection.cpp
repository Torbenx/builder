#include "Reflection.hpp"

Reflection::Reflection(Vec3 reflection,Vec3 position,int width,int height,float factor) :
    texture(Texture::color(width, height, Texture::Format::RGBA8888).move())
{
    this->reflection = reflection;
    this->position = position;
    this->factor = factor;
    this->width = width;
    this->height = height;
}

void Reflection::getReflectionModel(Mat4x4& model){
    model = Mat4x4::reflection(reflection,position);
}

void Reflection::init(){
    renderbuffer.storageDepth16(width, height);
    framebuffer.colorAttachment(texture);
    framebuffer.depthAttachment(renderbuffer);
    framebuffer.viewport(width, height);
}

void Reflection::bind(){
    framebuffer.bind();
}

void Reflection::addTo(LShaderBuilder& s){
    LShader::EffectData data;
    s.newContext();
    std::string textureUniform = s.uniqueName("u_texture");
    std::string positionVarying = s.uniqueName("v_position");
    s.addVarying("vec4", positionVarying);
    data.handles[0] = s.addFragmentUniform("sampler2D", textureUniform);
    data.textureSlot = s.nextTextureSlot();
    std::string color = s.uniqueName("color");
    s.vMain << positionVarying << " = " << LShader::SCREEN_POSITION << ";\n";
    char buff[100];
    sprintf(buff, "%f", factor);
    s.fMain << "vec3 " << color <<" = "<< buff << " * texture2D(" << textureUniform << ",(" << positionVarying << ".xy/" << positionVarying << ".w+1.0)/2.0).rgb;\n";
    s.fragmentColor = color;
    s.addEffect(*this, data);
}

void Reflection::apply(LShader& s, const  LShader::EffectData& data){
    s.uniformTexture(data.handles[0], texture, data.textureSlot);
}
