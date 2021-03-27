#ifndef HEADER_LGRAPHIC_DEPTHMAP_H
#define HEADER_LGRAPHIC_DEPTHMAP_H

#include "../graphic/Shader.hpp"
#include "../graphic/Framebuffer.hpp"
#include "../util/Renderable.hpp"
#include <string.h>

class DepthMap{
    public:
        Texture texture;
        Renderbuffer renderbuffer;
        Framebuffer framebuffer;
        int width,height;
        float m_far;
        Shader *shader;
        std::string fSource,vSource;
        DepthMap(int width,int height,float far);
        ~DepthMap();
        void init();
        void draw(MvpMatrix& mvpMatrix,Vec3 position,Renderable **ptrArray,int count);
};

#endif
