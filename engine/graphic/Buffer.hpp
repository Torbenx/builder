#ifndef HEADER_GRAPHIC_BUFFER_H
#define HEADER_GRAPHIC_BUFFER_H

#include "../gl/glwrapper.hpp"
#include <stdint.h>
#include <cstring>
#include <array>
#include "../math/Vec2.hpp"
#include "../math/Vec3.hpp"
#include "../math/Vec4.hpp"
#include "../util/ptrs.hpp"

enum WriteFrequency : GLenum{
    Normal = GL_STATIC_DRAW,
    Often = GL_DYNAMIC_DRAW,
};

template< typename T >
class Buffer{
public:

    static return_ptr<Buffer<T>> wrap(T *data, int size){
        return new Buffer<T>(data, size, false);
    }

    static return_ptr<Buffer<T>> adopt(T *data, int size){
        return new Buffer<T>(data, size, true);
    }

    static return_ptr<Buffer<T>> alloc(int size){
        return new Buffer<T>(new T[size], size, true);
    }

    static return_ptr<Buffer<T>> allocUp(int size, WriteFrequency wf=Normal){
        return upload(nullptr, size, wf);
    }

    static return_ptr<Buffer<T>> upload(T *data, int size, WriteFrequency wf=Normal){
        Buffer *b = Buffer::wrap(data, size).release();
        b->upload(wf);
        return b;
    }

    template< int N >
    static return_ptr<Buffer<T>> wrap(T (&data)[N]){
        return new Buffer<T>(data, N, false);
    }

    template< int N >
    static return_ptr<Buffer<T>> upload(T (&data)[N], WriteFrequency wf=Normal){
        Buffer *b = new Buffer<T>(data, N, false);
        b->upload(wf);
        return b;
    }

    template< std::size_t N >
    static return_ptr<Buffer<T>> wrap(std::array<T, N>& data){
        return new Buffer<T>(data.data(), N, false);
    }

    template< std::size_t N >
    static return_ptr<Buffer<T>> upload(std::array<T, N>& data, WriteFrequency wf=Normal){
        Buffer *b = new Buffer<T>(data.data(), N, false);
        b->upload(wf);
        return b;
    }

    template< int N >
    static return_ptr<Buffer<T>> wrap(T (&&data)[N]){
        return new Buffer<T>(data, N, false);
    }

    template< int N >
    static return_ptr<Buffer<T>> upload(T (&&data)[N], WriteFrequency wf=Normal){
        Buffer *b = new Buffer<T>(data, N, false);
        b->upload(wf);
        return b;
    }

    Buffer(Buffer<T>&& other) : isGLBuffer(other.isGL()), m_data(other.data()), m_owns(other.m_owns), glId(other.glId), m_size(other.size()){
        other.m_owns = false;
    }

    ~Buffer(){
        if(!owns()) return;
        if(isGL())
            glDeleteBuffers(1, &glId);
        else
            delete[] data();
    }

    return_ptr<Buffer<T>> view(int offset, int size){
        auto b = new Buffer<T>(*this);
        b->m_data += offset;
        b->m_size = size;
        return return_ptr(b);
    }

    bool isGL(){
        return isGLBuffer;
    }

    bool owns(){
        return m_owns;
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

    template< int N >
    void update(const T (&data)[N]){
        update(data, N);
    }

    template< std::size_t N >
    void update(const std::array<T, N>& data){
        update(data.data(), N);
    }

    template< int N >
    void update(const T (&&data)[N]){
        update(data, N);
    }

    void update(const T *data, int size){
        update(data, 0, size);
    }

    void update(const T *data, int offset, int size){
        if(isGL()){
            bind(GL_ARRAY_BUFFER);
            glBufferSubData(GL_ARRAY_BUFFER, (GLintptr)(this->data() + offset), size * sizeof(T), data);
        }else{
            std::memcpy(this->data()+offset, data, size * sizeof(T));
        }
    }

    void upload(WriteFrequency wf=Normal){
        GL_VERIFY(!isGL());
        isGLBuffer = true;
        glGenBuffers(1, &glId);
        bind(GL_ARRAY_BUFFER);
        glBufferData(GL_ARRAY_BUFFER, sizeInBytes(), m_data, wf);
        if(owns()){
            delete[] data();
        }
        m_data = nullptr;
        m_owns = true;
    }

    Buffer(int size) : Buffer(new T[size], size, true) {}

private:
    GLuint glId;
    T *m_data;
    GLsizeiptr m_size;
    bool isGLBuffer;
    bool m_owns;
    Buffer(T *data, int size, bool owns) : isGLBuffer(false), m_data(data), m_owns(owns), m_size(size), glId(0) {}
    Buffer(Buffer<T>& other) : isGLBuffer(other.isGL()), m_data(other.data()), m_owns(false), glId(other.glId), m_size(other.size()) {}
};

template class Buffer<float>;
template class Buffer<Vec2>;
template class Buffer<Vec3>;
template class Buffer<Vec4>;

template class Buffer<GLuint>;
template class Buffer<GLushort>;
template class Buffer<GLubyte>;

#endif
