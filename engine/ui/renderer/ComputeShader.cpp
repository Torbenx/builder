#include "ui/renderer/ComputeShader.hpp"
#include <bit>

namespace ui_renderer {

ComputeShader::ComputeShader(int_t layers) {
    ShaderBuilder builder;
    builder.makeCompute(32, 32);
    baseTexture = builder.addTextureUniform("uimage2D", "u_base", "restrict ", "rgba8ui");
    //builder.addTextureUniform("uimage2D", "u_atlas", "restrict, readonly ");

    builder.vInit << "struct Data {\n";
    builder.vInit << "ivec4 ids[" << (layers / 4 + 1) << "];\n"; // ids[0] holds the textureCount
    builder.vInit << "};\n";

    tileUniform = builder.beginVertexUniformBuffer("Tile");
    builder.vInit << "ivec2 u_baseOffset;\n";
    builder.vInit << "ivec2 u_begin;\n";
    builder.vInit << "Data u_data[64];\n";
    builder.endVertexUniformBuffer();

    /*builder.beginVertexUniformBuffer("Offsets");
    builder.vInit << "ivec2 u_offsets[];\n";
    builder.endVertexUniformBuffer();*/

    builder.vMain << "Data d = u_data[(u_begin.y + gl_WorkGroupID.y) * 8 + u_begin.x + gl_WorkGroupID.x];\n";
    builder.vMain << "ivec2 pos = u_baseOffset + ivec2(gl_GlobalInvocationID.xy);\n";
    builder.vMain << "uvec3 base = imageLoad(u_base, pos).rgb;\n";

    builder.vMain << "base = uvec3(50) * d.ids[0][0];\n";
    /*builder.vMain << "for(int i=1;i<=d.ids[0][0];i++){\n";
    builder.vMain << "uvec4 col = imageLoad(u_atlas, pos + u_offsets[d.ids[i / 4][i & 3]);\n";
    builder.vMain << "base = ((255 - col.a) * base + col.a * col.rgb) * 32897 >> 23;\n";
    builder.vMain << "}\n";*/

    builder.vMain << "imageStore(u_base, pos, uvec4(base, 255));\n";

    initialize(builder);
}

}