#include "Texture.hpp"
#include "../util/log.hpp"

constexpr int channelFormatLookupSize = Texture::Format::Count;

GLint formatsGL[channelFormatLookupSize];
GLenum formatsGLBase[channelFormatLookupSize];
GLenum formatsGLType[channelFormatLookupSize];

#define DEFINE_FORMAT(f, format, base_format, type) \
    formatsGL[f] = format; \
    formatsGLBase[f] = base_format; \
    formatsGLType[f] = type;

void init_textureFormats(){
    DEFINE_FORMAT(Texture::Format::RGBA8888, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);
}

GLint Texture::glFormatOf(Texture::Format f){
    return formatsGL[f];
}

GLenum Texture::glBaseFormatOf(Texture::Format f){
    return formatsGLBase[f];
}

GLenum Texture::glTypeOf(Texture::Format f){
    return formatsGLType[f];
}

return_ptr<Texture> Texture::depth(int width, int height, int bitdepth, bool renderingOnly){
    Texture *tex = new Texture(renderingOnly, width, height);
    GLenum format;
    if(bitdepth == 16){
        format = GL_DEPTH_COMPONENT16;
    }else{
        GL_EXPECT(bitdepth, 24);
        format = GL_DEPTH_COMPONENT24;
    }
    if(renderingOnly){
        glBindRenderbuffer(GL_RENDERBUFFER, tex->glId);
        glRenderbufferStorage(GL_RENDERBUFFER, format, width, height);
    }else{
        glBindTexture(GL_TEXTURE_2D, tex->glId);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
        tex->minifyFilter(Filter::Linear);
    }
    return tex;
}

return_ptr<Texture> Texture::stencil(int width, int height){
    Texture *tex = new Texture(true, width, height);
    glBindRenderbuffer(GL_RENDERBUFFER, tex->glId);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_STENCIL_INDEX8, width, height);
    return tex;
}

return_ptr<Texture> Texture::color(void *data, int width, int height, Texture::Format format){
    Texture *tex = new Texture(false, width, height);
    glBindTexture(GL_TEXTURE_2D, tex->glId);
    glTexImage2D(GL_TEXTURE_2D, 0, glBaseFormatOf(format), width, height, 0, glBaseFormatOf(format), glTypeOf(format), data);
    tex->minifyFilter(Filter::Linear);
    return tex;
}

return_ptr<Texture> Texture::color(int width, int height, Texture::Format format, bool renderingOnly){
    Texture *tex = new Texture(renderingOnly, width, height);
    if(renderingOnly){
        glBindRenderbuffer(GL_RENDERBUFFER, tex->glId);
        glRenderbufferStorage(GL_RENDERBUFFER, glFormatOf(format), width, height);
    }else{
        glBindTexture(GL_TEXTURE_2D, tex->glId);
        glTexImage2D(GL_TEXTURE_2D, 0, glFormatOf(format), width, height, 0, glBaseFormatOf(format), glTypeOf(format), nullptr);
        tex->minifyFilter(Filter::Linear);
    }
    return tex;
}

Texture::Texture(Texture&& other) :
    isRenderbuffer(other.isRenderbuffer), glId(other.glId), m_width(other.m_width), m_height(other.m_height), target(other.target), binding(other.binding)
{
    other.glId = 0;
}

Texture::Texture(bool isRenderbuffer, int width, int height, GLenum target, GLenum binding) :
    isRenderbuffer(isRenderbuffer), m_width(width), m_height(height), target(target), binding(binding)
{
    if(isRenderbuffer){
        glGenRenderbuffers(1, &glId);
    }else{
        glGenTextures(1, &glId);
    }
}

Texture::~Texture(){
    if(glId == 0){
        return;
    }
    if(isRenderbuffer){
        glDeleteRenderbuffers(1, &glId);
    }else{
        glDeleteTextures(1, &glId);
    }
}

void Texture::bind(){
    GL_VERIFY(!isRenderbuffer);
    glBindTexture(target, glId);
}

void Texture::magnifyFilter(Texture::Filter filter){
    GL_VERIFY(!isRenderbuffer);
    GL_GET_EXPECT(binding, glId);
    glTexParameteri(binding, GL_TEXTURE_MAG_FILTER, filter);
}

void Texture::minifyFilter(Texture::Filter filter){
    GL_VERIFY(!isRenderbuffer);
    GL_GET_EXPECT(binding, glId);
    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, filter);
}

void Texture::wrappingMode(Texture::WrappingMode xMode, Texture::WrappingMode yMode){
    GL_VERIFY(!isRenderbuffer);
    GL_GET_EXPECT(binding, glId);
    glTexParameteri(target, GL_TEXTURE_WRAP_S, xMode);
    glTexParameteri(target, GL_TEXTURE_WRAP_T, yMode);
}

void Texture::borderColor(Vec4 color){
    GL_VERIFY(!isRenderbuffer);
    GL_GET_EXPECT(binding, glId);
    glTexParameterfv(target, GL_TEXTURE_BORDER_COLOR, &color.x);
}

/*void Texture::subTexture(GLvoid *data,GLint xoffset,GLint yoffset,GLsizei width,GLsizei height,GLenum type){
    glTexSubImage2D(GL_TEXTURE_2D, 0, xoffset,yoffset,width,height,format,type,data);
}*/
