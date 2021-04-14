#include "GL/Framebuffer.hpp"
#include "util/log.hpp"

Framebuffer Framebuffer::DEFAULT(true, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

Framebuffer* Framebuffer::currentFramebuffer = &Framebuffer::DEFAULT;

// must match Texture::Type
GLenum GL_ATTACHMENTS[] = { GL_COLOR_ATTACHMENT0, GL_DEPTH_ATTACHMENT, GL_STENCIL_ATTACHMENT };
GLenum GL_BUFFER_BITS[] = { GL_COLOR_BUFFER_BIT, GL_DEPTH_BUFFER_BIT, GL_STENCIL_BUFFER_BIT };

Framebuffer::Framebuffer(bool isDefault, GLbitfield clearMask)
    : isDefault(isDefault), clearMask(clearMask) {
    if (isDefault)
        glId = 0;
    else
        glGenFramebuffers(1, &glId);
}

Framebuffer::~Framebuffer() {
    glDeleteFramebuffers(1, &glId);
}

void Framebuffer::bind() {
    GL_GET_EXPECT(GL_FRAMEBUFFER_BINDING, currentFramebuffer->glId);
#ifdef VALIDATE_GL
    /*if (isCurrent())
        LOGD("framebuffer %d already bound", glId);*/
#endif

    glBindFramebuffer(GL_FRAMEBUFFER, glId);
    currentFramebuffer = this;
    glClearColor(m_clearColor.x, m_clearColor.y, m_clearColor.z, m_clearColor.w);
    glViewport(offsetX, offsetY, width, height);
    glDepthMask(dMask);
    glColorMask(rMask, gMask, bMask, aMask);
    glStencilMask(sMask);
}

void Framebuffer::clear() {
    VALIDATE_FRAMEBUFFER_STATE
    glClear(clearMask);
}

void Framebuffer::draw(DrawMode mode, const IndexBuffer& indices) {
    VALIDATE_FRAMEBUFFER_STATE
    if (indices.hasData()) {
        indices.bind(GL_ELEMENT_ARRAY_BUFFER);
        GLenum type;
        switch (indices.elementSize()) {
        case 1:
            type = GL_UNSIGNED_BYTE;
            break;
        case 2:
            type = GL_UNSIGNED_SHORT;
            break;
        case 4:
            type = GL_UNSIGNED_INT;
            break;
        default:
            GL_VERIFY_NOT_REACHED();
        }
        glDrawElements(mode, indices.size(), type, indices.data());
    } else {
        glDrawArrays(mode, 0, indices.size());
    }
}

void Framebuffer::draw(DrawMode mode, int count) {
    VALIDATE_FRAMEBUFFER_STATE
    glDrawArrays(mode, 0, count);
}

void Framebuffer::attach(const Texture& texture) {
    attachInteral(texture, texture.target);
}

void Framebuffer::attach(const CubeMap& cubeMap, CubeMap::Face face) {
    attachInteral(cubeMap, face);
}

void Framebuffer::attachInteral(const Texture& texture, GLenum subTarget) {
    GL_VERIFY(isCurrent());
    clearMask |= GL_BUFFER_BITS[texture.type];
    if (texture.isRenderbuffer)
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_ATTACHMENTS[texture.type], GL_RENDERBUFFER, texture.glId);
    else
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_ATTACHMENTS[texture.type], subTarget, texture.glId, 0);
}

void Framebuffer::clearColor(float r, float g, float b, float a) {
    m_clearColor.set(r, g, b, a);
    if (isCurrent())
        glClearColor(r, g, b, a);
}

void Framebuffer::clearColor(Vec4 color) {
    m_clearColor = color;
    if (isCurrent())
        glClearColor(color.x, color.y, color.z, color.w);
}

void Framebuffer::viewport(int width, int height) {
    viewport(0, 0, width, height);
}

void Framebuffer::viewport(int offsetX, int offsetY, int width, int height) {
    this->offsetX = offsetX;
    this->offsetY = offsetY;
    this->width = width;
    this->height = height;
    if (isCurrent())
        glViewport(offsetX, offsetY, width, height);
}

void Framebuffer::colorMask(bool r, bool g, bool b, bool a) {
    rMask = r;
    gMask = g;
    bMask = b;
    aMask = a;
    if (isCurrent())
        glColorMask(r, g, b, a);
}

void Framebuffer::depthMask(bool d) {
    dMask = d;
    if (isCurrent())
        glDepthMask(d);
}

void Framebuffer::stencilMask(unsigned int s) {
    sMask = s;
    if (isCurrent())
        glStencilMask(s);
}
