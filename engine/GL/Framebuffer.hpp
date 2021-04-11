#pragma once

#include "GL/CubeMap.hpp"
#include "GL/Texture.hpp"
#include "math/Vec4.hpp"

#define VALIDATE_FRAMEBUFFER_STATE \
    GL_VERIFY(isCurrent());        \
    GL_EXPECT(glCheckFramebufferStatus(GL_FRAMEBUFFER), GL_FRAMEBUFFER_COMPLETE);

enum DrawMode : GLenum {
    Points = GL_POINTS,
    Lines = GL_LINES,
    LineStrip = GL_LINE_STRIP,
    LineLoop = GL_LINE_LOOP,
    Triangles = GL_TRIANGLES,
    TriangleStrip = GL_TRIANGLE_STRIP,
    TriangleFan = GL_TRIANGLE_FAN,
};

class Framebuffer {
public:
    static Framebuffer DEFAULT;

    Framebuffer()
        : Framebuffer(false, 0) { }
    ~Framebuffer();
    void bind(bool restoreAttachments = true);

    void clear();

    void draw(DrawMode mode, const IndexBuffer& indices);
    void draw(DrawMode mode, int count);

    void attach(const Texture& a);
    void attach(const CubeMap& a, CubeMap::Face);

    void clearColor(float r, float g, float b, float a);
    void clearColor(Vec4 color);
    void viewport(int width, int height);
    void viewport(int offsetX, int offsetY, int width, int height);
    void colorMask(bool r, bool g, bool b, bool a);
    void depthMask(bool d);
    void stencilMask(GLbitfield s);

private:
    static Framebuffer* currentFramebuffer;

    struct {
        Texture texture;
        GLenum subTarget;
    } attachments[3];
    GLuint glId;
    Vec4 m_clearColor;
    bool isDefault;
    bool rMask = true, gMask = true, bMask = true, aMask = true, dMask = true;
    GLbitfield sMask = 0xffffffff;
    GLbitfield clearMask = 0;
    int offsetY, offsetX, width, height;

    Framebuffer(bool isDefault, GLbitfield clearMask);
    Framebuffer(const Framebuffer& other);

    void attachInteral(const Texture& texture, GLenum subTarget);
    void applyAttchment(Texture::Type type);
    bool isCurrent() { return this == currentFramebuffer; }
};
