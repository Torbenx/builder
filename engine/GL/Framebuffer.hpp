#pragma once

#include "GL/Texture.hpp"
#include "math/Vec4.hpp"
#include "GL/CubeMap.hpp"

template< typename T >
struct IndexTraits { inline static GLenum glType; };

template<> struct IndexTraits<GLuint> { inline static GLenum glType = GL_UNSIGNED_INT; };
template<> struct IndexTraits<GLushort> { inline static GLenum glType = GL_UNSIGNED_SHORT; };
template<> struct IndexTraits<GLubyte> { inline static GLenum glType = GL_UNSIGNED_BYTE; };

#define VALIDATE_FRAMEBUFFER_STATE \
    GL_VERIFY(isCurrent()); \
    GL_EXPECT(glCheckFramebufferStatus(GL_FRAMEBUFFER), GL_FRAMEBUFFER_COMPLETE);

enum DrawMode : GLenum{
    Points = GL_POINTS,
    Lines = GL_LINES,
    LineStrip = GL_LINE_STRIP,
    LineLoop = GL_LINE_LOOP,
    Triangles = GL_TRIANGLES,
    TriangleStrip = GL_TRIANGLE_STRIP,
    TriangleFan = GL_TRIANGLE_FAN,
};

class Framebuffer{
public:

    static Framebuffer DEFAULT;

    Framebuffer() : Framebuffer(false, 0) {}
    ~Framebuffer();
    void bind();

    void clear();

    template< typename T >
    void draw(DrawMode mode, Buffer<T>& indices){
        VALIDATE_FRAMEBUFFER_STATE
        indices.bind(GL_ELEMENT_ARRAY_BUFFER);
        glDrawElements(mode, indices.size(), IndexTraits<T>::glType, indices.data());
    }

    void draw(DrawMode mode, int count){
        VALIDATE_FRAMEBUFFER_STATE
        glDrawArrays(mode, 0, count);
    }


    void attach(Texture& a);
    void attach(CubeMap& a, CubeMap::Face);

    void clearColor(float r, float g, float b, float a);
    void clearColor(Vec4 color);
    void viewport(int width, int height);
    void viewport(int offsetX, int offsetY, int width, int height);
    void colorMask(bool r, bool g, bool b, bool a);
    void depthMask(bool d);
    void stencilMask(GLbitfield s);

private:
    static Framebuffer *currentFramebuffer;

    struct {
        Texture *texture;
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

    void attachInteral(Texture& texture, GLenum subTarget);
    void applyAttchment(Texture::Type type);
    bool isCurrent() { return this == currentFramebuffer; }
};
