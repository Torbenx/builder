#include "lgraphic/DepthCubeMap.hpp"

DepthCubeMap::DepthCubeMap(int width, int height, float _near, float _far)
    : CubeMap(CubeMap::depth(width, height, ::Texture::DepthBits::D16).move()), m_far(_far), m_near(_near) {
    shader = Shaders::depth();
    framebuffer.viewport(width, height);
    projection = Mat4x4::frustum(-m_near, m_near, -m_near, m_near, m_near, m_far);
}

void DepthCubeMap::draw(Vec3 position, void (*doDraw)(Framebuffer& framebuffer, Shader& shader, Shader::LocationHandle positionAttrib, Shader::LocationHandle modelUniform)) {
    GL_VERIFY(glIsEnabled(GL_DEPTH_TEST));
    framebuffer.bind(false);
    shader->use();
    for (auto face : CubeMap::Faces) {
        Mat4x4 view;
        switch (face) {
        case CubeMap::PosX:
            view = Mat4x4::look(position, Vec3(+1, 0, 0), Vec3(0, -1, 0));
            break;
        case CubeMap::NegX:
            view = Mat4x4::look(position, Vec3(-1, 0, 0), Vec3(0, -1, 0));
            break;
        case CubeMap::PosY:
            view = Mat4x4::look(position, Vec3(0, +1, 0), Vec3(0, 0, +1));
            break;
        case CubeMap::NegY:
            view = Mat4x4::look(position, Vec3(0, -1, 0), Vec3(0, 0, -1));
            break;
        case CubeMap::PosZ:
            view = Mat4x4::look(position, Vec3(0, 0, +1), Vec3(0, -1, 0));
            break;
        case CubeMap::NegZ:
            view = Mat4x4::look(position, Vec3(0, 0, -1), Vec3(0, -1, 0));
            break;
        }
        view = projection * view;
        shader->uniformMat4x4(shader->mvpUniform, view);
        framebuffer.attach(*this, face);
        framebuffer.clear();
        doDraw(framebuffer, *shader, shader->positionAttrib, shader->modelUniform);
    }
}
