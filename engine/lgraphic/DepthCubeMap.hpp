#pragma once

#include "GL/Shader.hpp"
#include "GL/Framebuffer.hpp"
#include "util/Renderable.hpp"
#include "util/Shaders.hpp"
#include "math/MvpMatrix.hpp"

class DepthCubeMap{
    public:
        CubeMap texture;
        Texture depthAttachment;
        Framebuffer framebuffer;
        float m_far;
        std::shared_ptr<SimpleDepthShader> shader;
        DepthCubeMap(int width, int height, float far);
        void draw(Mat4x4 model, Vec3 position, Renderable **ptrArray, int count);
    private:
        Mat4x4 projection = Mat4x4();
        GLenum targets[6] = {GL_TEXTURE_CUBE_MAP_POSITIVE_X,GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
            GL_TEXTURE_CUBE_MAP_POSITIVE_Y,GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
            GL_TEXTURE_CUBE_MAP_POSITIVE_Z,GL_TEXTURE_CUBE_MAP_NEGATIVE_Z};

};
