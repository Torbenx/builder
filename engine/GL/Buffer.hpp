#pragma once

#include "GL/forward.hpp"
#include "GL/glwrapper.hpp"
#include "math/Vec2.hpp"
#include "math/Vec3.hpp"
#include "math/Vec4.hpp"
#include "util/log.hpp"
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
    static Buffer<BaseType> wrap(T* data, int size) {
        return construct<T>(data, size);
    }

    template<typename T>
    static Buffer<BaseType> alloc(int size) {
        return construct<T>(new T[size], size);
    }

    template<typename T>
    static Buffer<BaseType> allocUp(int size, WriteFrequency wf = Normal) {
        return upload<T>(nullptr, size, wf);
    }

    template<typename T>
    static Buffer<BaseType> upload(T* data, int size, WriteFrequency wf = Normal) {
        Buffer<BaseType> b = Buffer::wrap<T>(data, size);
        return Buffer<BaseType>::upload(b, wf);
    }

    template<typename T, int N>
    static Buffer<BaseType> wrap(T (&data)[N]) {
        return construct<T>(data, N);
    }

    template<typename T, int N>
    static Buffer<BaseType> upload(T (&data)[N], WriteFrequency wf = Normal) {
        Buffer<BaseType> b = construct<T>(data, N);
        return Buffer<BaseType>::upload(b, wf);
    }

    template<typename T, std::size_t N>
    static Buffer<BaseType> wrap(std::array<T, N>& data) {
        return construct<T>(data.data(), N);
    }

    template<typename T, std::size_t N>
    static Buffer<BaseType> upload(std::array<T, N>& data, WriteFrequency wf = Normal) {
        Buffer<BaseType> b = construct<T>(data.data(), N);
        return Buffer<BaseType>::upload(b, wf);
    }

    template<typename T, int N>
    static Buffer<BaseType> wrap(T(&&data)[N]) {
        return construct<T>(data, N);
    }

    template<typename T, int N>
    static Buffer<BaseType> upload(T(&&data)[N], WriteFrequency wf = Normal) {
        Buffer<BaseType> b = construct<T>(data, N);
        return Buffer<BaseType>::upload(b, wf);
    }

    static Buffer<BaseType> wrap(BaseType* data, int elementSize, int size) {
        return Buffer<BaseType>(data, elementSize, size);
    }

    static Buffer<BaseType> upload(BaseType* data, int elementSize, int size, WriteFrequency wf = Normal) {
        Buffer<BaseType> b(data, elementSize, size);
        return Buffer<BaseType>::upload(b, wf);
    }

    static Buffer<BaseType> upload(Buffer<BaseType>& in, WriteFrequency wf = Normal) {
        VERIFY(GL, !in.isGL());
        Buffer<BaseType> out(in);
        glGenBuffers(1, &out.glId);
        out.bind(GL_ARRAY_BUFFER);
        glBufferData(GL_ARRAY_BUFFER, in.sizeInBytes(), in.data(), wf);
        out.m_data = nullptr;
        return out;
    }

    static Buffer<BaseType> withoutData(int size) {
        return Buffer<BaseType>(nullptr, 0, size);
    }

    Buffer()
        : glId(0), m_data(nullptr), m_elementSize(0), m_size(0) { }

    void release() {
        VERIFY(GL, hasData());
        if (isGL())
            glDeleteBuffers(1, &glId);
        else
            delete[] data();
        glId = 0;
        m_data = nullptr;
        m_elementSize = 0;
        m_size = 0;
    }

    void view(int offset, int size) {
        m_data += offset * m_elementSize;
        m_size = size;
    }

    bool hasData() const { return m_elementSize > 0; }

    bool isGL() const { return glId != 0; }

    BaseType* data() const { return m_data; }

    int size() const { return m_size; }

    int sizeInBytes() const { return m_size * m_elementSize * sizeof(BaseType); }

    int elementSize() const { return m_elementSize; }

    void bind(GLenum target) const {
        glBindBuffer(target, glId);
    }

    template<typename T, int N>
    void update(const T (&data)[N]) const {
        update<T>(data, N);
    }

    template<typename T, std::size_t N>
    void update(const std::array<T, N>& data) const {
        update<T>(data.data(), N);
    }

    template<typename T, int N>
    void update(const T(&&data)[N]) const {
        update<T>(data, N);
    }

    template<typename T>
    void update(const T* data, int size) const {
        update(reinterpret_cast<const BaseType*>(data), 0, size * ElementTaits<T>::size);
    }

    void update(const BaseType* data, int offset, int size) const {
        VERIFY(GL, hasData());
        if (isGL()) {
            bind(GL_ARRAY_BUFFER);
            glBufferSubData(GL_ARRAY_BUFFER, (GLintptr)(this->data() + offset), size * sizeof(BaseType), data);
        } else {
            std::memcpy(this->data() + offset, data, size * sizeof(BaseType));
        }
    }

private:
    GLuint glId;
    BaseType* m_data;
    int m_size;
    int m_elementSize;

    template<typename T>
    static Buffer construct(T* data, int size) {
        return Buffer<BaseType>(reinterpret_cast<BaseType*>(data), ElementTaits<T>::size, size);
    }

    Buffer(BaseType* data, int m_elementSize, int size)
        : m_data(data), glId(0), m_elementSize(m_elementSize), m_size(size) { }
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
