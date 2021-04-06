#pragma once

#include "GL/glwrapper.hpp"
#include "math/Vec2.hpp"
#include "math/Vec3.hpp"
#include "math/Vec4.hpp"
#include "util/ptrs.hpp"
#include <array>
#include <cstring>
#include <stdint.h>

enum WriteFrequency : GLenum {
    Normal = GL_STATIC_DRAW,
    Often = GL_DYNAMIC_DRAW,
};

template<typename BaseType>
class Buffer {
public:
    template<typename T>
    struct ElementTaits {
        static const int size;
    };

    template<typename T>
    static return_ptr<Buffer<BaseType>> wrap(T* data, int size) {
        return construct<T>(data, size, false);
    }

    template<typename T>
    static return_ptr<Buffer<BaseType>> adopt(T* data, int size) {
        return construct<T>(data, size, true);
    }

    template<typename T>
    static return_ptr<Buffer<BaseType>> alloc(int size) {
        return construct<T>(new T[size], size, true);
    }

    template<typename T>
    static return_ptr<Buffer<BaseType>> allocUp(int size, WriteFrequency wf = Normal) {
        return upload<T>(nullptr, size, wf);
    }

    template<typename T>
    static return_ptr<Buffer<BaseType>> upload(T* data, int size, WriteFrequency wf = Normal) {
        Buffer* b = Buffer::wrap<T>(data, size).release();
        b->upload(wf);
        return b;
    }

    template<typename T, int N>
    static return_ptr<Buffer<BaseType>> wrap(T (&data)[N]) {
        return construct<T>(data, N, false);
    }

    template<typename T, int N>
    static return_ptr<Buffer<BaseType>> upload(T (&data)[N], WriteFrequency wf = Normal) {
        Buffer* b = construct<T>(data, N, false);
        b->upload(wf);
        return b;
    }

    template<typename T, std::size_t N>
    static return_ptr<Buffer<BaseType>> wrap(std::array<T, N>& data) {
        return construct<T>(data.data(), N, false);
    }

    template<typename T, std::size_t N>
    static return_ptr<Buffer<BaseType>> upload(std::array<T, N>& data, WriteFrequency wf = Normal) {
        Buffer* b = construct<T>(data.data(), N, false);
        b->upload(wf);
        return b;
    }

    template<typename T, int N>
    static return_ptr<Buffer<BaseType>> wrap(T(&&data)[N]) {
        return construct<T>(data, N, false);
    }

    template<typename T, int N>
    static return_ptr<Buffer<BaseType>> upload(T(&&data)[N], WriteFrequency wf = Normal) {
        Buffer* b = construct<T>(data, N, false);
        b->upload(wf);
        return b;
    }

    Buffer(Buffer<BaseType>&& other)
        : isGLBuffer(other.isGL()), m_data(other.data()), glId(other.glId), m_owns(other.m_owns), m_elementSize(other.m_elementSize), m_size(other.size()) {
        other.m_owns = false;
    }

    ~Buffer() {
        if (!owns())
            return;
        if (isGL())
            glDeleteBuffers(1, &glId);
        else
            delete[] data();
    }

    return_ptr<Buffer<BaseType>> view(int offset, int size) {
        auto b = new Buffer<BaseType>(*this);
        b->m_data += offset * m_elementSize;
        b->m_size = size;
        return return_ptr(b);
    }

    bool isGL() { return isGLBuffer; }

    bool owns() { return m_owns; }

    BaseType* data() { return m_data; }

    int size() { return m_size; }

    int sizeInBytes() { return m_size * m_elementSize * sizeof(BaseType); }

    int elementSize() { return m_elementSize; }

    void bind(GLenum target) {
        glBindBuffer(target, glId);
    }

    template<typename T, int N>
    void update(const T (&data)[N]) {
        update<T>(data, N);
    }

    template<typename T, std::size_t N>
    void update(const std::array<T, N>& data) {
        update<T>(data.data(), N);
    }

    template<typename T, int N>
    void update(const T(&&data)[N]) {
        update<T>(data, N);
    }

    template<typename T>
    void update(const T* data, int size) {
        update(reinterpret_cast<const BaseType*>(data), 0, size * ElementTaits<T>::size);
    }

    void update(const BaseType* data, int offset, int size) {
        if (isGL()) {
            bind(GL_ARRAY_BUFFER);
            glBufferSubData(GL_ARRAY_BUFFER, (GLintptr)(this->data() + offset), size * sizeof(BaseType), data);
        } else {
            std::memcpy(this->data() + offset, data, size * sizeof(BaseType));
        }
    }

    void upload(WriteFrequency wf = Normal) {
        GL_VERIFY(!isGL());
        isGLBuffer = true;
        glGenBuffers(1, &glId);
        bind(GL_ARRAY_BUFFER);
        glBufferData(GL_ARRAY_BUFFER, sizeInBytes(), data(), wf);
        if (owns()) {
            delete[] data();
        }
        m_data = nullptr;
        m_owns = true;
    }

private:
    GLuint glId;
    BaseType* m_data;
    int m_size;
    bool isGLBuffer;
    bool m_owns;
    int m_elementSize;

    template<typename T>
    static Buffer* construct(T* data, int size, bool owns) {
        return new Buffer<BaseType>(reinterpret_cast<BaseType*>(data), ElementTaits<T>::size, size, owns);
    }

    Buffer(BaseType* data, int m_elementSize, int size, bool owns)
        : isGLBuffer(false), m_data(data), glId(0), m_owns(owns), m_elementSize(m_elementSize), m_size(size) { }

    Buffer(Buffer<BaseType>& other)
        : isGLBuffer(other.isGL()), m_data(other.data()), glId(other.glId), m_owns(false), m_elementSize(other.m_elementSize), m_size(other.size()) { }
};

template class Buffer<float>;
template class Buffer<GLubyte>;

typedef Buffer<float> VertexBuffer;
typedef Buffer<GLubyte> IndexBuffer;

template<>
template<>
struct VertexBuffer::ElementTaits<float> { static const int size = 1; };
template<>
template<>
struct VertexBuffer::ElementTaits<Vec2> { static const int size = 2; };
template<>
template<>
struct VertexBuffer::ElementTaits<Vec3> { static const int size = 3; };
template<>
template<>
struct VertexBuffer::ElementTaits<Vec4> { static const int size = 4; };

template<>
template<>
struct IndexBuffer::ElementTaits<GLubyte> { static const int size = 1; };
template<>
template<>
struct IndexBuffer::ElementTaits<GLushort> { static const int size = 2; };
template<>
template<>
struct IndexBuffer::ElementTaits<GLuint> { static const int size = 4; };
