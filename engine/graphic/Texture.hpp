#ifndef HEADER_GRAPHIC_TEXTURE_H
#define HEADER_GRAPHIC_TEXTURE_H

#include "../gl/glwrapper.hpp"
#include "Buffer.hpp"
#include "forward.hpp"

class Texture{
public:
    enum Format{
        RGBA8888,
        Count = RGBA8888+1
    };

    static GLint glFormatOf(Format);
    static GLenum glBaseFormatOf(Format);
    static GLenum glTypeOf(Format);

    static Texture* depth(int bitdepth, int width, int height, bool renderingOnly=false);
    static Texture* stencil(int width, int height);
    static Texture* color(void *data, int width, int height, Format format, Format targetFormat);
    static Texture* color(void *data, int width, int height, Format format);
    static Texture* color(int width, int height, Format format, bool renderingOnly=false);

    /*GLfloat minFilter = GL_LINEAR;
    GLfloat magFilter = GL_LINEAR;
    GLfloat wrapS = GL_CLAMP_TO_EDGE;
    GLfloat wrapT = GL_CLAMP_TO_EDGE;*/

    Texture(Texture&&);
    virtual ~Texture();
    void bind();
    //void subTexture(GLvoid *data, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum type);
    void setTexParameter(GLfloat minfilter, GLfloat magfilter, GLfloat wraps, GLfloat wrapt);
    void applyTexParameter();
    int width(){ return m_width; }
    int height(){ return m_height; }

protected:
    Texture(bool isRenderbuffer, int width, int height);
    GLuint glId;
    friend Framebuffer;

private:
    Texture(const Texture& other);
    bool isRenderbuffer;
    int m_width, m_height;
};

#endif
