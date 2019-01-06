#ifndef HEADER_GRAPHIC_CUBEMAP_H
#define HEADER_GRAPHIC_CUBEMAP_H

#include "../gl/glwrapper.hpp"
#include "../util/SourceGetter.hpp"

class CubeMap{
    public:
        SourceGetter<void*> *posX,*negX,*posY,*negY,*posZ,*negZ;
        GLuint glId;
        GLfloat minFilter = GL_LINEAR;
        GLfloat magFilter = GL_LINEAR;
        GLfloat wrapS = GL_CLAMP_TO_EDGE;
        GLfloat wrapT = GL_CLAMP_TO_EDGE;
        GLsizei posXwidth,posXheight,negXwidth,negXheight,posYwidth,posYheight,negYwidth,negYheight,posZwidth,posZheight,negZwidth,negZheight;
        GLenum posXformat,posXtype,negXformat,negXtype,posYformat,posYtype,negYformat,negYtype,posZformat,posZtype,negZformat,negZtype;
        CubeMap();
        ~CubeMap();
        bool init();
        void bind();
        void texturePosX(SourceGetter<void*> *data,GLsizei width,GLsizei height,GLenum format,GLenum type);
        void textureNegX(SourceGetter<void*> *data,GLsizei width,GLsizei height,GLenum format,GLenum type);
        void texturePosY(SourceGetter<void*> *data,GLsizei width,GLsizei height,GLenum format,GLenum type);
        void textureNegY(SourceGetter<void*> *data,GLsizei width,GLsizei height,GLenum format,GLenum type);
        void texturePosZ(SourceGetter<void*> *data,GLsizei width,GLsizei height,GLenum format,GLenum type);
        void textureNegZ(SourceGetter<void*> *data,GLsizei width,GLsizei height,GLenum format,GLenum type);
        void applyTexParameter();
        void setTexParameter(GLfloat minfilter,GLfloat magfilter,GLfloat wraps,GLfloat wrapt);
    private:
        CubeMap(const CubeMap& other);
        void texture(GLenum target,SourceGetter<void*> *data,GLsizei width,GLsizei height,GLenum format,GLenum type);
};

#endif
