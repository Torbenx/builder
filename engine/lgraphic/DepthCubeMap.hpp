#pragma once

#include "GL/Shader.hpp"
#include "GL/Framebuffer.hpp"
#include "util/Renderable.hpp"
#include "util/Shaders.hpp"

class DepthCubeMap : public CubeMap{
public:
    Framebuffer framebuffer;
    float m_far, m_near;
    DepthCubeMap(int width, int height, float near, float far);
    void draw(Vec3 position, void (*doDraw)(Framebuffer& framebuffer, Shader& shader, Shader::LocationHandle positionAttrib, Shader::LocationHandle modelUniform));
private:
    std::shared_ptr<SimpleDepthShader> shader;
    Mat4x4 projection;

};
