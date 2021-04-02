#include "util/Shaders.hpp"

void SimpleTextureShader::applySimpleTextureShader(ShaderBuilder& b, const std::string& sampler, const std::string& texFunc){
    positionAttrib = b.addAttrib("vec4", "a_position");
    texCoordAttrib = b.addAttrib("vec2", "a_texCoord");
    textureUniform = b.addFragmentUniform(sampler, "u_texture");
    b.addVarying("vec2", "v_texCoord");
    b.vMain <<
    "   v_texCoord = a_texCoord;\n"
    "   gl_Position = a_position;\n";
    b.fMain <<
    "   gl_FragColor = " << texFunc << "(u_texture, v_texCoord);\n";
}

void SimpleTextShader::applySimpleTextShader(ShaderBuilder& b){
    b.fInit << TEXT_TEXTURE_GLSL_HEADER;
    applySimpleTextureShader(b, TEXT_TEXTURE_GLSL_SAMPLER, "texture");
}

void SimpleDepthShader::applySimpleDepthShader(ShaderBuilder& b){
    positionAttrib = b.addAttrib("vec4", "a_position");
    mvpUniform = b.addVertexUniform("mat4", "u_mvpMatrix");
    modelUniform = b.addVertexUniform("mat4", "u_modelMatrix");
    lightPosUniform = b.addFragmentUniform("vec3", "u_lightPos");
    farUniform = b.addFragmentUniform("float", "u_far");
    b.addVarying("vec3", "v_position");
    b.vMain <<
    "    vec4 pos = u_modelMatrix * a_position;\n"
    "    v_position = pos.xyz;\n"
    "    gl_Position = u_mvpMatrix * pos;\n";
    b.fMain <<
    "    float f0 = distance(u_lightPos, v_position)*255.0/u_far;\n"
    "    float f1 = fract(f0);\n"
    "    f0 -= f1;\n"
    "    f1 *= 255.0;\n"
    "    float f2 = fract(f1);\n"
    "    f1 -= f2;\n"
    "    f2 *= 255.0;\n"
    "    float f3 = fract(f2);\n"
    "    f2 -= f3;\n"
    "    f3 *= 255.0;\n"
    "    f3 = ceil(f3);\n"
    "    gl_FragColor = vec4(f0,f1,f2,f3)/255.0;\n";
}

void SingleColorShader::applySingleColorShader(ShaderBuilder& b){
    positionAttrib = b.addAttrib("vec4", "a_position");
    colorUniform = b.addFragmentUniform("vec4", "u_color");
    b.vMain << "gl_Position = a_position;\n";
    b.fMain << "gl_FragColor = u_color;\n";
}

void SimpleCubeMapShader::applySimpleCubeMapShader(ShaderBuilder& b, const std::string& sampler, const std::string& texFunc){
    positionAttrib = b.addAttrib("vec4", "a_position");
    texCoordAttrib = b.addAttrib("vec3", "a_texCoord");
    cubeMapUniform = b.addFragmentUniform(sampler, "u_cubeMap");
    b.addVarying("vec3", "v_texCoord");
    b.vMain <<
    "   v_texCoord = a_texCoord;\n"
    "   gl_Position = a_position;\n";
    b.fMain <<
    "   gl_FragColor = " << texFunc << "(u_cubeMap, v_texCoord);\n";
}

void SimpleDepthMapShader::applySimpleDepthMapShader(ShaderBuilder& b){
    applySimpleTextureShader(b, "sampler2D", "texture");
    farUniform = b.addFragmentUniform("float", "u_far");
    b.fMain <<
    "   vec4 bits = vec4(u_far,u_far/255.0,u_far/255.0/255.0,u_far/255.0/255.0/255.0);\n"
    "   gl_FragColor = vec4(vec3(dot(bits,gl_FragColor)), 1.0);\n";
}

void SimpleDepthCubeMapShader::applySimpleDepthCubeMapShader(ShaderBuilder& b){
    applySimpleCubeMapShader(b, "samplerCube", "textureCube");
    farUniform = b.addFragmentUniform("float", "u_far");
    b.fMain <<
    "   vec4 bits = vec4(u_far,u_far/255.0,u_far/255.0/255.0,u_far/255.0/255.0/255.0);\n"
    "   gl_FragColor = vec4(vec3(dot(bits,gl_FragColor)), 1.0);\n";
}
