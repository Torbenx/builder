#ifndef HEADER_GRAPHIC_BUFFER_H
#define HEADER_GRAPHIC_BUFFER_H

#include "../gl/glwrapper.hpp"
#include <stdint.h>
#include <cstring>

enum WriteFrequency : GLenum{
    Rare = GL_STATIC_DRAW,
    Often = GL_DYNAMIC_DRAW,
};

template< typename T >
class Buffer{
    public:

        static Buffer<T>* wrap(T *data, int size){
            return new Buffer(data, size, false);
        }

        static Buffer<T>* adopt(T *data, int size){
            return new Buffer(data, size, true);
        }

        Buffer(T *data, int size, bool owns) :
            isGLBuffer(false), m_size(size)
        {
            impl.mem.data = data;
            impl.mem.owns = owns;
        }

        ~Buffer(){
            if(isGL())
                glDeleteBuffers(1, &impl.gl.glId);
            else if(impl.mem.owns){
                delete[] impl.mem.data;
            }
        }

        bool isGL(){
            return isGLBuffer;
        }

        T* data(){
            return impl.mem.data;
        }

        int size(){
            return m_size;
        }

        int sizeInBytes(){
            return m_size * sizeof(T);
        }

        void bind(GLenum target){
            glBindBuffer(target, impl.gl.glId);
        }

        void update(T *data, int offset, int size){
            if(isGL()){
                bind(GL_ARRAY_BUFFER);
                glBufferSubData(GL_ARRAY_BUFFER, offset * sizeof(T), size * sizeof(T), data);
            }else{
                std::memcpy(impl.mem.data+offset, data, size * sizeof(T));
            }
        }

        Buffer<T>* upload(WriteFrequency wf){
            return new Buffer<T>(data(), size(), wf);
        }

    private:
        bool isGLBuffer;
        union{
            struct{
                GLuint glId;
            } gl;
            struct {
                T *data;
                bool owns;
            } mem;
        } impl;
        GLsizeiptr m_size;

        Buffer(const Buffer<T>& other);
        Buffer(T *data, int size, WriteFrequency wf) :
            isGLBuffer(true), m_size(size)
        {
            glGenBuffers(1, &impl.gl.glId);
            bind(GL_ARRAY_BUFFER);
            glBufferData(GL_ARRAY_BUFFER, this->size() * sizeof(T), data, wf);
        }
};

template class Buffer<float>;
template class Buffer<Vec2>;
template class Buffer<Vec3>;
template class Buffer<Vec4>;

template class Buffer<GLuint>;
template class Buffer<GLushort>;

#endif
