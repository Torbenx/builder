#ifndef HEADER_GRAPHIC_FRAMEBUFFER_H
#define HEADER_GRAPHIC_FRAMEBUFFER_H

#include "Renderbuffer.hpp"
#include "Texture.hpp"
#include "../math/Vec4.hpp"
#include "CubeMap.hpp"

class Framebuffer{
    public:
        static Framebuffer DEFAULT;
        GLuint glId = -1;
        Texture *colorTex = 0,*depthTex = 0,*stencilTex = 0;
        Renderbuffer *colorBuffer = 0,*depthBuffer = 0,*stencilBuffer = 0;
        CubeMap *colorCube = 0,*depthCube = 0,*stencilCube = 0;
        GLenum colorCubeTarget,depthCubeTarget,stencilCubeTarget;
        Vec4 clearColor;
        int offsetY,offsetX,width,height;
        bool isDefault,rMask = true,gMask = true,bMask = true,aMask = true,dMask = true;
        unsigned int sMask = 0xffffffff;
        Framebuffer();
        ~Framebuffer();
        void bind();
        bool init();
        void texture(GLenum attachment,Texture *a);
        void cubeMap(GLenum attachment,GLenum target,CubeMap *a);
        void renderbuffer(GLenum attachment,Renderbuffer *a);
        void colorAttachment(Texture& a);
        void colorAttachment(Renderbuffer& a);
        void colorAttachment(GLenum target,CubeMap& a);
        void depthAttachment(Texture& a);
        void depthAttachment(Renderbuffer& a);
        void depthAttachment(GLenum target,CubeMap& a);
        void stencilAttachment(Texture& a);
        void stencilAttachment(Renderbuffer& a);
        void stencilAttachment(GLenum target,CubeMap& a);
        void setClearColor(float r,float g,float b,float a);
        void setClearColor(Vec4 color);
        void viewport(int width,int height);
        void viewport(int offsetX,int offsetY,int width,int height);
        void colorMask(bool r,bool g,bool b,bool a);
        void depthMask(bool d);
        void stencilMask(unsigned int s);
    private:
        Framebuffer(bool isDefault);
};

#endif
