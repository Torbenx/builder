#pragma once

#include "GL/glwrapper.hpp"

class CubeMap{
    public:
        void *posX,*negX,*posY,*negY,*posZ,*negZ;
        GLuint glId = -1;
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
        void texturePosX(void* data,GLsizei width,GLsizei height,GLenum format,GLenum type);
        void textureNegX(void* data,GLsizei width,GLsizei height,GLenum format,GLenum type);
        void texturePosY(void* data,GLsizei width,GLsizei height,GLenum format,GLenum type);
        void textureNegY(void* data,GLsizei width,GLsizei height,GLenum format,GLenum type);
        void texturePosZ(void* data,GLsizei width,GLsizei height,GLenum format,GLenum type);
        void textureNegZ(void* data,GLsizei width,GLsizei height,GLenum format,GLenum type);
        void applyTexParameter();
        void setTexParameter(GLfloat minfilter,GLfloat magfilter,GLfloat wraps,GLfloat wrapt);
    private:
        CubeMap(const CubeMap& other);
        void texture(GLenum target,void* data,GLsizei width,GLsizei height,GLenum format,GLenum type);
};
