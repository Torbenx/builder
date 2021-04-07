#include "lgraphic/DepthMap.hpp"
#include <stdio.h>

DepthMap::DepthMap(int width, int height)
    : Texture(Texture::depth(width, height, Texture::DepthBits::D16).move()), shader(Shaders::depth()) {
    framebuffer.attach(*this);
    framebuffer.viewport(width, height);
}

void DepthMap::draw(Mat4x4& vpMatrix, void (*doDraw)(Framebuffer& framebuffer, Shader& shader, Shader::LocationHandle positionAttrib, Shader::LocationHandle modelUniform)) {
    GL_VERIFY(glIsEnabled(GL_DEPTH_TEST));
    framebuffer.bind();
    framebuffer.clear();
    shader->use();
    shader->uniformMat4x4(shader->mvpUniform, vpMatrix);
    doDraw(framebuffer, *shader, shader->positionAttrib, shader->modelUniform);
}
