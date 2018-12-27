#ifndef HEADER_GRAPHIC_TEXTURE_H
#define HEADER_GRAPHIC_TEXTURE_H

#include "../gl/glwrapper.hpp"
#include "../util/SourceGetter.hpp"

class Texture{
    public:
        GLuint glId;
        GLfloat minFilter = GL_LINEAR;
        GLfloat magFilter = GL_LINEAR;
        GLfloat wrapS = GL_CLAMP_TO_EDGE;
        GLfloat wrapT = GL_CLAMP_TO_EDGE;
        GLenum format,type;
        SourceGetter<void*> *source;
        GLsizei width,height;
        Texture();
        ~Texture();
        bool init();
        void bind();
        void texture(SourceGetter<void*> *data,GLsizei width,GLsizei height,GLenum format,GLenum type);
        void subTexture(GLvoid *data,GLint xoffset,GLint yoffset,GLsizei width,GLsizei height,GLenum type);
        void setTexParameter(GLfloat minfilter,GLfloat magfilter,GLfloat wraps,GLfloat wrapt);
        void applyTexParameter();
};

#endif
