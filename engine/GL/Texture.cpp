#include "GL/Texture.hpp"
#include "util/log.hpp"

constexpr int channelFormatLookupSize = (int)Texture::Format::Count;

GLint formatsGL[channelFormatLookupSize];
GLenum formatsGLBase[channelFormatLookupSize];
GLenum formatsGLType[channelFormatLookupSize];

#define DEFINE_FORMAT(f, format, base_format, type) \
    formatsGL[(int)f] = format;                          \
    formatsGLBase[(int)f] = base_format;                 \
    formatsGLType[(int)f] = type;

void init_textureFormats() {
    DEFINE_FORMAT(Texture::Format::RGBA8888, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);
}

GLint Texture::glFormatOf(Texture::Format f) {
    return formatsGL[(int)f];
}

GLenum Texture::glBaseFormatOf(Texture::Format f) {
    return formatsGLBase[(int)f];
}

GLenum Texture::glTypeOf(Texture::Format f) {
    return formatsGLType[(int)f];
}

Texture Texture::depth(int width, int height, DepthBits bits, bool renderingOnly) {
    Texture tex(Type::Depth, renderingOnly);
    if (renderingOnly) {
        glBindRenderbuffer(GL_RENDERBUFFER, tex.glId);
        glRenderbufferStorage(GL_RENDERBUFFER, bits, width, height);
    } else {
        glBindTexture(GL_TEXTURE_2D, tex.glId);
        glTexImage2D(GL_TEXTURE_2D, 0, bits, width, height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
        tex.minifyFilter(Filter::Linear);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    }
    return tex;
}

Texture Texture::stencil(int width, int height) {
    Texture tex(Type::Stencil, true);
    glBindRenderbuffer(GL_RENDERBUFFER, tex.glId);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_STENCIL_INDEX8, width, height);
    return tex;
}

Texture Texture::color(void* data, int width, int height, Texture::Format format) {
    Texture tex(Type::Color, false);
    glBindTexture(GL_TEXTURE_2D, tex.glId);
    glTexImage2D(GL_TEXTURE_2D, 0, glBaseFormatOf(format), width, height, 0, glBaseFormatOf(format), glTypeOf(format), data);
    tex.minifyFilter(Filter::Linear);
    return tex;
}

Texture Texture::color(int width, int height, Texture::Format format, bool renderingOnly) {
    Texture tex(Type::Color, renderingOnly);
    if (renderingOnly) {
        glBindRenderbuffer(GL_RENDERBUFFER, tex.glId);
        glRenderbufferStorage(GL_RENDERBUFFER, glFormatOf(format), width, height);
    } else {
        glBindTexture(GL_TEXTURE_2D, tex.glId);
        glTexImage2D(GL_TEXTURE_2D, 0, glFormatOf(format), width, height, 0, glBaseFormatOf(format), glTypeOf(format), nullptr);
        tex.minifyFilter(Filter::Linear);
    }
    return tex;
}

Texture::Texture(Type type, bool isRenderbuffer, GLenum target, GLenum binding)
    : target(target), binding(binding), type(type), isRenderbuffer(isRenderbuffer) {
    if (isRenderbuffer) {
        glGenRenderbuffers(1, &glId);
    } else {
        glGenTextures(1, &glId);
    }
}

void Texture::release() {
    VERIFY(GL, glId != 0);
    VERIFY(GL, glIsTexture(glId));
    if (isRenderbuffer) {
        glDeleteRenderbuffers(1, &glId);
    } else {
        glDeleteTextures(1, &glId);
    }
    glId = 0;
}

void Texture::bind() const {
    VERIFY(GL, !isRenderbuffer);
    glBindTexture(target, glId);
}

void Texture::magnifyFilter(Texture::Filter filter) const {
    VERIFY(GL, !isRenderbuffer);
    GL_GET_EXPECT(binding, glId);
    glTexParameteri(binding, GL_TEXTURE_MAG_FILTER, filter);
}

void Texture::minifyFilter(Texture::Filter filter) const {
    VERIFY(GL, !isRenderbuffer);
    GL_GET_EXPECT(binding, glId);
    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, filter);
}

void Texture::wrappingMode(Texture::WrappingMode xMode, Texture::WrappingMode yMode) const {
    VERIFY(GL, !isRenderbuffer);
    GL_GET_EXPECT(binding, glId);
    glTexParameteri(target, GL_TEXTURE_WRAP_S, xMode);
    glTexParameteri(target, GL_TEXTURE_WRAP_T, yMode);
}

void Texture::borderColor(Vec4 color) const {
    VERIFY(GL, !isRenderbuffer);
    GL_GET_EXPECT(binding, glId);
    glTexParameterfv(target, GL_TEXTURE_BORDER_COLOR, &color.x);
}

/*void Texture::subTexture(GLvoid *data,GLint xoffset,GLint yoffset,GLsizei width,GLsizei height,GLenum type){
    glTexSubImage2D(GL_TEXTURE_2D, 0, xoffset,yoffset,width,height,format,type,data);
}*/
