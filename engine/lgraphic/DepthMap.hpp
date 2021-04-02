#pragma once

#include "GL/Shader.hpp"
#include "GL/Framebuffer.hpp"
#include "util/Renderable.hpp"
#include "math/MvpMatrix.hpp"
#include "util/Shaders.hpp"
#include <string.h>

class DepthMap : public Texture{
    public:
        Texture depthAttachment;
        Framebuffer framebuffer;
        float m_far;
        std::shared_ptr<SimpleDepthShader> shader;
        DepthMap(int width, int height, float far);
        ~DepthMap();
        void draw(MvpMatrix& mvpMatrix, Vec3 position, Renderable **ptrArray, int count);
};
