#include "util/Shaders.hpp"

void SimpleTextureShader::applySimpleTextureShader(ShaderBuilder& b, const std::string& sampler){
    positionAttrib = b.addAttrib("vec4", "a_position");
    texCoordAttrib = b.addAttrib("vec2", "a_texCoord");
    textureUniform = b.addTextureUniform(sampler, "u_texture");
    b.addVarying("vec2", "v_texCoord");
    b.vMain <<
    "   v_texCoord = a_texCoord;\n"
    "   gl_Position = a_position;\n";
    b.fMain <<
    "   " << Shader::FRAGMENT_COLOR << " = texture(u_texture, v_texCoord);\n";
}

void SimpleTextShader::applySimpleTextShader(ShaderBuilder& b){
#ifdef TEXT_TEXTURE_GLSL_EXTENSION
    b.requireExtension(TEXT_TEXTURE_GLSL_EXTENSION);
#endif
    applySimpleTextureShader(b, TEXT_TEXTURE_GLSL_SAMPLER);
}

void SimpleDepthShader::applySimpleDepthShader(ShaderBuilder& b){
    positionAttrib = b.addAttrib("vec4", "a_position");
    mvpUniform = b.addVertexUniform("mat4", "u_mvpMatrix");
    b.vMain <<"    gl_Position = u_mvpMatrix * a_position;\n";
    b.disableFragmentShader();
}

void SingleColorShader::applySingleColorShader(ShaderBuilder& b){
    positionAttrib = b.addAttrib("vec4", "a_position");
    colorUniform = b.addFragmentUniform("vec4", "u_color");
    b.vMain << "gl_Position = a_position;\n";
    b.fMain << "" << Shader::FRAGMENT_COLOR << " = u_color;\n";
}

void SimpleCubeMapShader::applySimpleCubeMapShader(ShaderBuilder& b, const std::string& sampler){
    positionAttrib = b.addAttrib("vec4", "a_position");
    texCoordAttrib = b.addAttrib("vec3", "a_texCoord");
    cubeMapUniform = b.addTextureUniform(sampler, "u_cubeMap");
    b.addVarying("vec3", "v_texCoord");
    b.vMain <<
    "   v_texCoord = a_texCoord;\n"
    "   gl_Position = a_position;\n";
    b.fMain <<
    "   " << Shader::FRAGMENT_COLOR << " = texture(u_cubeMap, v_texCoord);\n";
}
