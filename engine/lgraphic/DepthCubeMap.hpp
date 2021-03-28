#ifndef HEADER_LGRAPHIC_DEPTHCUBEMAP_H
#define HEADER_LGRAPHIC_DEPTHCUBEMAP_H

#include "../graphic/Shader.hpp"
#include "../graphic/Framebuffer.hpp"
#include "../util/Renderable.hpp"
#include "../math/MvpMatrix.hpp"
#include "DepthMap.hpp"

class DepthCubeMap{
    public:
        CubeMap texture;
        Renderbuffer renderbuffer;
        Framebuffer framebuffer;
        int width,height;
        float m_far;
        std::unique_ptr<DepthMap::DepthShader> shader;
        std::string vSource,fSource;
        DepthCubeMap(int width,int height,float far);
        ~DepthCubeMap();
        void init();
        void draw(Mat4x4 model,Vec3 position,Renderable **ptrArray,int count);
    private:
        Mat4x4 projection = Mat4x4();
        GLenum targets[6] = {GL_TEXTURE_CUBE_MAP_POSITIVE_X,GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
            GL_TEXTURE_CUBE_MAP_POSITIVE_Y,GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
            GL_TEXTURE_CUBE_MAP_POSITIVE_Z,GL_TEXTURE_CUBE_MAP_NEGATIVE_Z};

};

#endif
