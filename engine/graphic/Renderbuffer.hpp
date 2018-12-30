#ifndef HEADER_GRAPHIC_RENDERBUFFER_H
#define HEADER_GRAPHIC_RENDERBUFFER_H

#include "../gl/glwrapper.hpp"
#include "../util/Handle.hpp"

class Renderbuffer{
    public:
        GLsizei width,height;
        GLenum format;
        Handle<GLuint> glId;
        Renderbuffer();
        ~Renderbuffer();
        bool init();
        void bind();
        void storage(GLenum format,GLsizei width,GLsizei height);
        void storageRGBA5551(GLsizei width,GLsizei height);
        void storageRGBA4444(GLsizei width,GLsizei height);
        void storageDepth16(GLsizei width,GLsizei height);
        void storageStencil8(GLsizei width,GLsizei height);
};

#endif
