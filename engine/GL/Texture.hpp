#pragma once

#include "GL/glwrapper.hpp"
#include "GL/Buffer.hpp"
#include "GL/forward.hpp"
#include "util/ptrs.hpp"

class Texture{
public:
    enum Format{
        RGBA8888,
        Count = RGBA8888+1
    };

    static GLint glFormatOf(Format);
    static GLenum glBaseFormatOf(Format);
    static GLenum glTypeOf(Format);

    enum WrappingMode : GLint{
        Clamp = GL_CLAMP_TO_EDGE,
        Repeat = GL_REPEAT,
        MirroredRepeat = GL_MIRRORED_REPEAT,
        ClampBorder = GL_CLAMP_TO_BORDER,
    };

    enum Filter : GLint{
        Linear = GL_LINEAR,
        Nearest = GL_NEAREST,
    };

    static return_ptr<Texture> depth(int width, int height, int bitdepth, bool renderingOnly=false);
    static return_ptr<Texture> stencil(int width, int height);
    static return_ptr<Texture> color(void *data, int width, int height, Format format, Format targetFormat);
    static return_ptr<Texture> color(void *data, int width, int height, Format format);
    static return_ptr<Texture> color(int width, int height, Format format, bool renderingOnly=false);

    Texture(Texture&&);
    virtual ~Texture();
    void bind();

    void wrappingMode(WrappingMode xMode, WrappingMode yMode); //default value is WrappingMode::Repeat
    void magnifyFilter(Filter); //default value is Filter::Linear
    void minifyFilter(Filter); //default value is Filter::Linear (GL default this to GL_NEAREST_MIPMAP_LINEAR so we have to override for non mipmaps)
    void borderColor(Vec4 color); //default value is Vec4(0,0,0,0)

    //void subTexture(GLvoid *data, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum type);

    int width(){ return m_width; }
    int height(){ return m_height; }

protected:
    Texture(bool isRenderbuffer, int width, int height, GLenum target=GL_TEXTURE_2D, GLenum binding=GL_TEXTURE_BINDING_2D);
    GLuint glId;
    GLenum target;
    GLenum binding;
    friend Framebuffer;

private:
    Texture(const Texture& other);
    bool isRenderbuffer;
    int m_width, m_height;
};
