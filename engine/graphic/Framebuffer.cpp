#include "Framebuffer.hpp"
#include "../util/log.hpp"

Framebuffer Framebuffer::DEFAULT(true, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

Framebuffer *Framebuffer::currentFramebuffer = &Framebuffer::DEFAULT;

#define VALIDATE_FRAMEBUFFER_STATE \
    GL_VERIFY(isCurrent()); \
    GL_EXPECT(glCheckFramebufferStatus(GL_FRAMEBUFFER), GL_FRAMEBUFFER_COMPLETE);

Framebuffer::Framebuffer(bool isDefault, GLbitfield clearMask) : isDefault(isDefault), clearMask(clearMask){
    if(isDefault)
        glId = 0;
    else
        glGenFramebuffers(1, &glId);
}

Framebuffer::~Framebuffer(){
    glDeleteFramebuffers(1, &glId);
}

void Framebuffer::bind(){
#ifdef VALIDATE_GL
    GLint fbo = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &fbo);
    GL_EXPECT(fbo, currentFramebuffer->glId);
    if(isCurrent()) LOGD("framebuffer %d already bound", glId);
#endif

    glBindFramebuffer(GL_FRAMEBUFFER, glId);
    currentFramebuffer = this;
    attach(GL_COLOR_ATTACHMENT0, m_colorAttachment);
    attach(GL_DEPTH_ATTACHMENT, m_depthAttachment);
    attach(GL_STENCIL_ATTACHMENT, m_stencilAttachment);
    GL_EXPECT(glCheckFramebufferStatus(GL_FRAMEBUFFER), GL_FRAMEBUFFER_COMPLETE);
    glClearColor(m_clearColor.x, m_clearColor.y, m_clearColor.z, m_clearColor.w);
    glViewport(offsetX, offsetY, width, height);
    glDepthMask(dMask);
    glColorMask(rMask, gMask, bMask, aMask);
    glStencilMask(sMask);
}

void Framebuffer::clear(){
    VALIDATE_FRAMEBUFFER_STATE
    glClear(clearMask);
}

bool Framebuffer::attach(GLenum attachment, Attachment& att){
    switch(att.type){
        case Attachment::Type::Empty:
            return false;
        case Attachment::Type::TextureAttachemnt:
            glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, att.data.texture->glId, 0);
            return true;
        case Attachment::Type::RenderbufferAttachment:
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, att.data.renderbuffer->glId);
            return true;
        case Attachment::Type::CubeMapAttachment:
            glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, att.data.cube.target, att.data.cube.cubeMap->glId, 0);
            return true;
        default:
            LOGE("invalid attchment type!!!");
            return false;
    }
}

void Framebuffer::colorAttachment(Texture& a){
    m_colorAttachment.type = Attachment::Type::TextureAttachemnt;
    m_colorAttachment.data.texture = &a;
    if(isCurrent()) attach(GL_COLOR_ATTACHMENT0, m_colorAttachment);
    clearMask |= GL_COLOR_BUFFER_BIT;
}

void Framebuffer::colorAttachment(Renderbuffer& a){
    m_colorAttachment.type = Attachment::Type::RenderbufferAttachment;
    m_colorAttachment.data.renderbuffer = &a;
    if(isCurrent()) attach(GL_COLOR_ATTACHMENT0, m_colorAttachment);
    clearMask |= GL_COLOR_BUFFER_BIT;
}

void Framebuffer::colorAttachment(GLenum target, CubeMap& a){
    m_colorAttachment.type = Attachment::Type::CubeMapAttachment;
    m_colorAttachment.data.cube.cubeMap = &a;
    m_colorAttachment.data.cube.target = target;
    if(isCurrent()) attach(GL_COLOR_ATTACHMENT0, m_colorAttachment);
    clearMask |= GL_COLOR_BUFFER_BIT;
}

void Framebuffer::depthAttachment(Texture& a){
    m_depthAttachment.type = Attachment::Type::TextureAttachemnt;
    m_depthAttachment.data.texture = &a;
    if(isCurrent()) attach(GL_DEPTH_ATTACHMENT, m_depthAttachment);
    clearMask |= GL_DEPTH_BUFFER_BIT;
}

void Framebuffer::depthAttachment(Renderbuffer& a){
    m_depthAttachment.type = Attachment::Type::RenderbufferAttachment;
    m_depthAttachment.data.renderbuffer = &a;
    if(isCurrent()) attach(GL_DEPTH_ATTACHMENT, m_depthAttachment);
    clearMask |= GL_DEPTH_BUFFER_BIT;
}

void Framebuffer::depthAttachment(GLenum target, CubeMap& a){
    m_depthAttachment.type = Attachment::Type::CubeMapAttachment;
    m_depthAttachment.data.cube.cubeMap = &a;
    m_depthAttachment.data.cube.target = target;
    if(isCurrent()) attach(GL_DEPTH_ATTACHMENT, m_depthAttachment);
    clearMask |= GL_DEPTH_BUFFER_BIT;
}

void Framebuffer::stencilAttachment(Texture& a){
    m_stencilAttachment.type = Attachment::Type::TextureAttachemnt;
    m_stencilAttachment.data.texture = &a;
    if(isCurrent()) attach(GL_STENCIL_ATTACHMENT, m_stencilAttachment);
    clearMask |= GL_STENCIL_BUFFER_BIT;
}

void Framebuffer::stencilAttachment(Renderbuffer& a){
    m_stencilAttachment.type = Attachment::Type::RenderbufferAttachment;
    m_stencilAttachment.data.renderbuffer = &a;
    if(isCurrent()) attach(GL_STENCIL_ATTACHMENT, m_stencilAttachment);
    clearMask |= GL_STENCIL_BUFFER_BIT;
}

void Framebuffer::stencilAttachment(GLenum target, CubeMap& a){
    m_stencilAttachment.type = Attachment::Type::CubeMapAttachment;
    m_stencilAttachment.data.cube.cubeMap = &a;
    m_stencilAttachment.data.cube.target = target;
    if(isCurrent()) attach(GL_STENCIL_ATTACHMENT, m_stencilAttachment);
    clearMask |= GL_STENCIL_BUFFER_BIT;
}

void Framebuffer::clearColor(float r, float g, float b, float a){
    m_clearColor.set(r, g, b, a);
    if(isCurrent()) glClearColor(r, g, b, a);
}

void Framebuffer::clearColor(Vec4 color){
    m_clearColor = color;
    if(isCurrent()) glClearColor(color.x, color.y, color.z, color.w);
}

void Framebuffer::viewport(int width, int height){
    viewport(0, 0, width, height);
}

void Framebuffer::viewport(int offsetX, int offsetY, int width, int height){
    this->offsetX = offsetX;
    this->offsetY = offsetY;
    this->width = width;
    this->height = height;
    if(isCurrent()) glViewport(offsetX, offsetY, width, height);
}

void Framebuffer::colorMask(bool r,bool g,bool b,bool a){
    rMask = r;
    gMask = g;
    bMask = b;
    aMask = a;
    if(isCurrent()) glColorMask(r, g, b, a);
}

void Framebuffer::depthMask(bool d){
    dMask = d;
    if(isCurrent()) glDepthMask(d);
}

void Framebuffer::stencilMask(unsigned int s){
    sMask = s;
    if(isCurrent()) glStencilMask(s);
}
