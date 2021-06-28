#pragma once

#include "GL/Buffer.hpp"
#include "GL/forward.hpp"
#include "GL/glwrapper.hpp"

class Texture {
public:
    enum Type {
        Color = 0,
        Depth = 1,
        Stencil = 2,
    };

    enum Format {
        RGBA8888,
        Count = RGBA8888 + 1
    };
    static GLint glFormatOf(Format);
    static GLenum glBaseFormatOf(Format);
    static GLenum glTypeOf(Format);

    enum DepthBits : GLint {
        D16 = GL_DEPTH_COMPONENT16,
        D24 = GL_DEPTH_COMPONENT24,
    };

    enum WrappingMode : GLint {
        Clamp = GL_CLAMP_TO_EDGE,
        Repeat = GL_REPEAT,
        MirroredRepeat = GL_MIRRORED_REPEAT,
        ClampBorder = GL_CLAMP_TO_BORDER,
    };

    enum Filter : GLint {
        Linear = GL_LINEAR,
        Nearest = GL_NEAREST,
    };

    static Texture depth(int width, int height, DepthBits bits, bool renderingOnly = false);
    static Texture stencil(int width, int height);
    static Texture color(void* data, int width, int height, Format format, Format targetFormat);
    static Texture color(void* data, int width, int height, Format format);
    static Texture color(int width, int height, Format format, bool renderingOnly = false);

    Texture()
        : glId(0) { }
    Texture(const Texture&);

    void bind() const;
    void wrappingMode(WrappingMode xMode, WrappingMode yMode) const; // default value is WrappingMode::Repeat
    void magnifyFilter(Filter) const; // default value is Filter::Linear
    void minifyFilter(Filter) const; // default value is Filter::Linear (GL defaults this to GL_NEAREST_MIPMAP_LINEAR so we have to override for non mipmaps)
    void borderColor(Vec4 color) const; // default value is Vec4(0,0,0,0)
    bool isValid() const { return glId != 0; }

    // void subTexture(GLvoid *data, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum type);
    void release();

protected:
    Texture(Type type, bool isRenderbuffer, GLenum target = GL_TEXTURE_2D, GLenum binding = GL_TEXTURE_BINDING_2D);
    GLuint glId;
    GLenum target;
    GLenum binding;
    Type type;
    bool isRenderbuffer;
    friend Framebuffer;
};
