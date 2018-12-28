#include "Framebuffer.hpp"
#include "../util/log.hpp"

Framebuffer Framebuffer::DEFAULT(true);

Framebuffer::Framebuffer(){
    
}

Framebuffer::Framebuffer(bool isDefault){
    this->isDefault = isDefault;
    glId = 0;
}
    
Framebuffer::~Framebuffer(){
    glDeleteFramebuffers(1,&glId);
}
    
void Framebuffer::bind(){
    GLint fbo = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING,&fbo);
    if(glId!=fbo){
        glBindFramebuffer(GL_FRAMEBUFFER,glId);
    }
    if(colorTex){
        texture(GL_COLOR_ATTACHMENT0,colorTex);
    }else if(colorBuffer){
        renderbuffer(GL_COLOR_ATTACHMENT0,colorBuffer);
    }else if(colorCube){
        cubeMap(GL_COLOR_ATTACHMENT0,colorCubeTarget,colorCube);
    }
    if(depthTex){
        texture(GL_DEPTH_ATTACHMENT,depthTex);
    }else if(depthBuffer){
        renderbuffer(GL_DEPTH_ATTACHMENT,depthBuffer);
    }else if(depthCube){
        cubeMap(GL_DEPTH_ATTACHMENT,depthCubeTarget,depthCube);
    }
    if(stencilTex){
        texture(GL_STENCIL_ATTACHMENT,stencilTex);
    }else if(stencilBuffer){
        renderbuffer(GL_STENCIL_ATTACHMENT,stencilBuffer);
    }else if(stencilCube){
        cubeMap(GL_STENCIL_ATTACHMENT,stencilCubeTarget,stencilCube);
    }
    int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(status!=GL_FRAMEBUFFER_COMPLETE){
        LOGE("framebuffer not ready: 0x%x",status);
    }
    glClearColor(clearColor.x, clearColor.y, clearColor.z, clearColor.w);
    glViewport(0,0,width,height);
    glDepthMask(dMask);
    glColorMask(rMask,gMask,bMask,aMask);
    glStencilMask(sMask);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
}
    
bool Framebuffer::init(){
    if((!isDefault)&&!glIsFramebuffer(glId)){
        glGenFramebuffers(1,&glId);
        return true;
    }
    return false;
}
    
void Framebuffer::texture(GLenum attachment,Texture *a){
    glFramebufferTexture2D(GL_FRAMEBUFFER,attachment,GL_TEXTURE_2D,a->glId,0);
}
    
void Framebuffer::renderbuffer(GLenum attachment,Renderbuffer *a){
    glFramebufferRenderbuffer(GL_FRAMEBUFFER,attachment,GL_RENDERBUFFER,a->glId);
}

void Framebuffer::cubeMap(GLenum attachment,GLenum target,CubeMap *a){
    glFramebufferTexture2D(GL_FRAMEBUFFER,attachment,target,a->glId,0);
}
    
void Framebuffer::colorAttachment(Texture& a){
    colorTex = &a;
    colorBuffer = 0;
    colorCube = 0;
}
    
void Framebuffer::colorAttachment(Renderbuffer& a){
    colorTex = 0;
    colorBuffer = &a;
    colorCube = 0;
}

void Framebuffer::colorAttachment(GLenum target,CubeMap& a){
    colorTex = 0;
    colorBuffer = 0;
    colorCube = &a;
    colorCubeTarget = target;
}
    
void Framebuffer::depthAttachment(Texture& a){
    depthTex = &a;
    depthBuffer = 0;
    depthCube = 0;
}
    
void Framebuffer::depthAttachment(Renderbuffer& a){
    depthTex = 0;
    depthBuffer = &a;
    depthCube = 0;
}

void Framebuffer::depthAttachment(GLenum target,CubeMap& a){
    depthTex = 0;
    depthBuffer = 0;
    depthCube = &a;
    depthCubeTarget = target;
}
    
void Framebuffer::stencilAttachment(Texture& a){
    stencilTex = &a;
    stencilBuffer = 0;
    stencilCube = 0;
}
    
void Framebuffer::stencilAttachment(Renderbuffer& a){
    stencilTex = 0;
    stencilBuffer = &a;
    stencilCube = 0;
}

void Framebuffer::stencilAttachment(GLenum target,CubeMap& a){
    stencilTex = 0;
    stencilBuffer = 0;
    stencilCube = &a;
    stencilCubeTarget = target;
}

void Framebuffer::setClearColor(float r,float g,float b,float a){
    clearColor.set(r,g,b,a);
}
        
void Framebuffer::setClearColor(Vec4 color){
    clearColor = color;
}
        
void Framebuffer::viewport(float width,float height){
    this->width = width;
    this->height = height;
}

void Framebuffer::colorMask(bool r,bool g,bool b,bool a){
    rMask = r;
    gMask = g;
    bMask = b;
    aMask = a;
}
        
void Framebuffer::depthMask(bool d){
    dMask = d;
}
        
void Framebuffer::stencilMask(unsigned int s){
    sMask = s;
}
