#ifndef HEADER_GRAPHIC_BUFFER_H
#define HEADER_GRAPHIC_BUFFER_H

#include "../gl/glwrapper.hpp"
#include <stdint.h>

const static GLint TE_BUFFER = 0x00cf;

template <typename T>
class Buffer{
    public:
        GLint target;
        GLint usage;
        GLuint glId;
        GLsizeiptr size;
        T *source;
        
        Buffer(GLint target){
            this->target = target;
        }
        
        ~Buffer(){
            glDeleteBuffers(1,&glId);
        }
        
        bool init(){
            if(target!=TE_BUFFER&&!glIsBuffer(glId)){
                glGenBuffers(1,&glId);
                return true;
            }
            return false;
        }
        
        void bind(){
            glBindBuffer(target,glId);
        }
        
        void bufferData(T* data,GLsizeiptr sizeInBytes,GLint usage){
            source = data;
            size = sizeInBytes;
            this->usage = usage;
            if(target!=TE_BUFFER){
                bind();
                glBufferData(target,sizeInBytes,data,usage);
            }
        }
        
        void bufferSubData(T *data,GLintptr offset,GLsizeiptr sizeInBytes){
            bind();
            glBufferSubData(target,offset,sizeInBytes,data);
        }
    private:
        Buffer(const Buffer& other);
};
#define Bufferf Buffer<float>
#define Bufferi Buffer<uint32_t>

template class Buffer<float>;
template class Buffer<uint32_t>;

#endif
