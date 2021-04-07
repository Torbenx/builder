#pragma once

#include "GL/Shader.hpp"
#include "GL/Framebuffer.hpp"
#include "util/Renderable.hpp"
#include "util/Shaders.hpp"
#include <string.h>

class DepthMap : public Texture{
public:
    Framebuffer framebuffer;
    DepthMap(int width, int height);
    void draw(Mat4x4& mvpMatrix, void (*doDraw)(Framebuffer& framebuffer, Shader& shader, Shader::LocationHandle positionAttrib, Shader::LocationHandle modelUniform));
private:
    std::shared_ptr<SimpleDepthShader> shader;
};
