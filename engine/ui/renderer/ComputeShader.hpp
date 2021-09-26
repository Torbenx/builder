#pragma once

#include "GL/Shader.hpp"

namespace ui_renderer {

class ComputeShader : public Shader {
public:
    LocationHandle tileUniform, baseTexture;
    ComputeShader(int_t maxTextures);
};

}