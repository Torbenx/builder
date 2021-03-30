#ifndef HEADER_GRAPHIC_BUFFER_H
#define HEADER_GRAPHIC_BUFFER_H

#include "../gl/glwrapper.hpp"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

class Buffer{
    public:

        enum WriteFrequency : GLenum{
            Rare = GL_STATIC_DRAW,
            Often = GL_DYNAMIC_DRAW,
        };

        static Buffer* wrap(void *data, int sizeInBytes){
            return new Buffer(data, sizeInBytes, false);
        }

        static Buffer* adopt(void *data, int sizeInBytes){
            return new Buffer(data, sizeInBytes, true);
        }

        Buffer(void *data, int sizeInBytes, bool owns) :
            isGLBuffer(false), m_size(sizeInBytes)
        {
            impl.mem.data = data;
            impl.mem.owns = owns;
        }

        ~Buffer(){
            if(isGL())
                glDeleteBuffers(1, &impl.gl.glId);
            else if(impl.mem.owns){
                free(impl.mem.data);
            }
        }

        bool isGL(){
            return isGLBuffer;
        }

        void* data(){
            return impl.mem.data;
        }

        int size(){
            return m_size;
        }

        void bind(GLenum target){
            glBindBuffer(target, impl.gl.glId);
        }

        void update(void *data, int offset, int sizeInBytes){
            if(isGL()){
                bind(GL_ARRAY_BUFFER);
                glBufferSubData(GL_ARRAY_BUFFER, offset, sizeInBytes, data);
            }else{
                memcpy(((uint8_t*)impl.mem.data)+offset, data, sizeInBytes);
            }
        }

        Buffer* upload(WriteFrequency wf){
            return new Buffer(data(), size(), wf);
        }

    private:
        bool isGLBuffer;
        union{
            struct{
                GLuint glId;
            } gl;
            struct {
                void *data;
                bool owns;
            } mem;
        } impl;
        GLsizeiptr m_size;

        Buffer(const Buffer& other);
        Buffer(void *data, int sizeInBytes, WriteFrequency wf) :
            isGLBuffer(true), m_size(sizeInBytes)
        {
            glGenBuffers(1, &impl.gl.glId);
            bind(GL_ARRAY_BUFFER);
            glBufferData(GL_ARRAY_BUFFER, size(), data, wf);
        }
};

#endif
