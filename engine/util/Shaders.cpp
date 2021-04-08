#include "util/Shaders.hpp"

void SimpleTextureShader::applySimpleTextureShader(ShaderBuilder& b, const std::string& sampler){
    positionAttrib = b.addAttrib("vec4", "a_position");
    texCoordAttrib = b.addAttrib("vec2", "a_texCoord");
    textureUniform = b.addFragmentUniform(sampler, "u_texture");
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
    modelUniform = b.addVertexUniform("mat4", "u_modelMatrix");
    b.vMain <<"    gl_Position = u_mvpMatrix * u_modelMatrix * a_position;\n";
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
    cubeMapUniform = b.addFragmentUniform(sampler, "u_cubeMap");
    b.addVarying("vec3", "v_texCoord");
    b.vMain <<
    "   v_texCoord = a_texCoord;\n"
    "   gl_Position = a_position;\n";
    b.fMain <<
    "   " << Shader::FRAGMENT_COLOR << " = texture(u_cubeMap, v_texCoord);\n";
}

void SimpleDepthMapShader::applySimpleDepthMapShader(ShaderBuilder& b){
    applySimpleTextureShader(b, "sampler2D");
    farUniform = b.addFragmentUniform("float", "u_far");
    b.fMain <<
    "   vec4 bits = vec4(u_far,u_far/255.0,u_far/255.0/255.0,u_far/255.0/255.0/255.0);\n"
    "   " << Shader::FRAGMENT_COLOR << " = vec4(vec3(dot(bits," << Shader::FRAGMENT_COLOR << ")), 1.0);\n";
}

void SimpleDepthCubeMapShader::applySimpleDepthCubeMapShader(ShaderBuilder& b){
    applySimpleCubeMapShader(b, "samplerCube");
    farUniform = b.addFragmentUniform("float", "u_far");
    b.fMain <<
    "   vec4 bits = vec4(u_far,u_far/255.0,u_far/255.0/255.0,u_far/255.0/255.0/255.0);\n"
    "   " << Shader::FRAGMENT_COLOR << " = vec4(vec3(dot(bits," << Shader::FRAGMENT_COLOR << ")), 1.0);\n";
}
