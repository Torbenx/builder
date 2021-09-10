#pragma once

#include "graphics/GAllocator.hpp"
#include "math/constants.hpp"

template<typename T = void>
struct GAllocationHelper {
    TypedGPtr<T> ptr;

    void release(GAllocator& allocator) {
        allocator.free(&ptr);
    }

    ~GAllocationHelper() {
        VERIFY(Graphics, ptr.bufferId == 0);
    }
};

static inline int_t uniformAlignment() {
    static GLint alignment = 0;
    if (alignment == 0) {
        glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &alignment);
    }
    return alignment;
}

template<typename T>
struct UniformHelper : GAllocationHelper<T> {

    UniformBuffer uniformBuffer(GAllocator& allocator) const {
        return { allocator.buffer(this->ptr), sizeof(T) };
    }

    T& allocateDynamic(GAllocator& allocator) {
        return *(T*)allocator.allocateDynamic(&this->ptr, uniformAlignment(), sizeof(T));
    }

    T& dynamicUpdate(GAllocator& allocator) {
        return *(T*)allocator.mapping(this->ptr);
    }

    T& staticUpdate(GAllocator& allocator) {
        return *(T*)allocator.allocateStatic(&this->ptr, uniformAlignment(), sizeof(T));
    }
};

template<typename T>
struct VertexAttribHelper : GAllocationHelper<T*> {
    VertexAttribBuffer vertexBuffer(GAllocator& allocator) const {
        return { allocator.buffer(this->ptr), VectorTraits<T>::length };
    }

    T* staticUpdate(GAllocator& allocator, int_t n) {
        return (T*)allocator.allocateStatic(&this->ptr, alignof(T), sizeof(T) * n);
    }

    void staticUpdate(GAllocator& allocator, const T* data, int_t n) {
        std::memcpy(staticUpdate(allocator, n), data, sizeof(T) * n);
    }

    template<int_t N>
    void staticUpdate(GAllocator& allocator, const T (&data)[N]) {
        staticUpdate(allocator, data, N);
    }

    template<size_t N>
    void staticUpdate(GAllocator& allocator, const std::array<T, N>& data) {
        staticUpdate(allocator, data.data(), N);
    }
};
