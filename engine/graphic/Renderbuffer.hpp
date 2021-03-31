#ifndef HEADER_GRAPHIC_RENDERBUFFER_H
#define HEADER_GRAPHIC_RENDERBUFFER_H

#include "../gl/glwrapper.hpp"
#include "forward.hpp"

class Renderbuffer{
    public:
        static Renderbuffer depth(int size);
        GLsizei width,height;
        GLenum format;
        Renderbuffer();
        ~Renderbuffer();
        void bind();
        void storage(GLenum format, GLsizei width, GLsizei height);
        void storageRGBA5551(GLsizei width, GLsizei height);
        void storageRGBA4444(GLsizei width, GLsizei height);
        void storageDepth16(GLsizei width, GLsizei height);
        void storageStencil8(GLsizei width, GLsizei height);
    private:
        GLuint glId;
        Renderbuffer(const Renderbuffer& other);
        friend Framebuffer;
};

#endif
