#ifndef HEADER_GRAPHIC_BUFFER_H
#define HEADER_GRAPHIC_BUFFER_H

#include "../gl/glwrapper.hpp"
#include "../util/SourceGetter.hpp"
#include "../util/Handle.hpp"
#include <stdint.h>

const static GLint TE_BUFFER = 0x00cf;

template <typename T>
class Buffer{
    public:
        GLint target;
        GLint usage;
        Handle<GLuint> glId;
        GLsizeiptr size;
        SourceGetter<T*> *source;
        
        Buffer(GLint target) : glId(-1){
            this->target = target;
        }
        
        ~Buffer(){
            if(glId.unref()){
                glDeleteBuffers(1,&glId.value);
            }
        }
        
        bool init(){
            if(target!=TE_BUFFER&&!glIsBuffer(glId.value)){
                if(glId.refCount != 1){
                    LOGW("init on copy");
                }
                glGenBuffers(1,&glId.value);
                return true;
            }
            return false;
        }
        
        void bind(){
            glBindBuffer(target,glId.value);
        }
        
        void bufferData(SourceGetter<T*> *data,GLsizeiptr sizeInBytes,GLint usage){
            source = data;
            size = sizeInBytes;
            this->usage = usage;
            if(target!=TE_BUFFER){
                bind();
                glBufferData(target,sizeInBytes,data->getSource(),usage);
            }
        }
        
        void bufferSubData(T *data,GLintptr offset,GLsizeiptr sizeInBytes){
            bind();
            glBufferSubData(target,offset,sizeInBytes,data);
        }
};
#define Bufferf Buffer<float>
#define Bufferi Buffer<uint32_t>

template class Buffer<float>;
template class Buffer<uint32_t>;

#endif
