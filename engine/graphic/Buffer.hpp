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
            isGLBuffer(false), m_data(data), m_owns(owns), m_size(size), glId(0) {}

        ~Buffer(){
            if(isGL())
                glDeleteBuffers(1, &glId);
            else if(m_owns)
                delete[] data();
        }

        bool isGL(){
            return isGLBuffer;
        }

        T* data(){
            return m_data;
        }

        int size(){
            return m_size;
        }

        int sizeInBytes(){
            return m_size * sizeof(T);
        }

        void bind(GLenum target){
            glBindBuffer(target, glId);
        }

        void update(T *data, int offset, int size){
            if(isGL()){
                bind(GL_ARRAY_BUFFER);
                glBufferSubData(GL_ARRAY_BUFFER, offset * sizeof(T), size * sizeof(T), data);
            }else{
                std::memcpy(this->data()+offset, data, size * sizeof(T));
            }
        }

        Buffer<T>* upload(WriteFrequency wf){
            return new Buffer<T>(data(), size(), wf);
        }

    private:
        GLuint glId;
        T *m_data;
        GLsizeiptr m_size;
        bool isGLBuffer;
        bool m_owns;

        Buffer(const Buffer<T>& other);
        Buffer(T *data, int size, WriteFrequency wf) :
            isGLBuffer(true), m_size(size), m_data(nullptr), m_owns(true)
        {
            glGenBuffers(1, &glId);
            bind(GL_ARRAY_BUFFER);
            glBufferData(GL_ARRAY_BUFFER, size * sizeof(T), data, wf);
        }
};

template class Buffer<float>;
template class Buffer<Vec2>;
template class Buffer<Vec3>;
template class Buffer<Vec4>;

template class Buffer<GLuint>;
template class Buffer<GLushort>;

#endif
