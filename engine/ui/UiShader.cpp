#include "ui/UiShader.hpp"

UiShader::UiShader(int_t maxTextures) {
    ShaderBuilder builder;
    builder.makeCompute(16, 16);
    builder.addVertexUniform("uimage2D", "u_base", "restrict ");
    builder.addVertexUniform("uimage2D", "u_in", "restrict, readonly ", maxTextures);
    builder.beginVertexUniformBuffer("TextureCount");
    builder.vInit << "ivec2 u_baseOffset;\n";
    builder.vInit << "int u_textureCount;\n";
    builder.vInit << "ivec2 u_inOffsets[" << maxTextures << "];\n";
    builder.endVertexUniformBuffer();

    builder.vMain << "ivec2 pos = ivec2(gl_LocalInvocationID.xy);\n";
    builder.vMain << "ivec2 basePos = u_baseOffset + pos;\n";
    builder.vMain << "uvec3 base = imageLoad(u_base, basePos).rgb;\n";
    builder.vMain << "for(int i=0;i<u_textureCount;i++){\n";
    builder.vMain << "uvec4 col = imageLoad(u_in[i], u_inOffsets[i] + pos);\n";
    builder.vMain << "base = ((255 - col.a) * base + col.a * col.rgb) * 32897 >> 23;\n";
    builder.vMain << "}\n";
    builder.vMain << "imageStore(u_base, basePos, uvec4(base, 255));\n";

    initialize(builder);
}