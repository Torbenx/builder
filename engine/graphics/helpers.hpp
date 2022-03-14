#pragma once

#include "graphics/GAllocator.hpp"
#include "math/constants.hpp"

template<typename T = void>
struct GAllocationHelper {
    TypedGPtr<T> ptr;

    void release(GAllocator* allocator) {
        allocator->free(&ptr);
    }

    ~GAllocationHelper() {
        VERIFY(Graphics, !ptr);
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
    UniformBuffer uniformBuffer(GAllocator* allocator) const {
        return { allocator->buffer(this->ptr), sizeof(T) };
    }

    T& allocateDynamic(GAllocator* allocator) {
        return *(T*)allocator->allocateDynamic(&this->ptr, uniformAlignment(), sizeof(T));
    }

    T& dynamicUpdate(GAllocator* allocator) {
        return *(T*)allocator->mapping(this->ptr);
    }

    T& staticUpdate(GAllocator* allocator) {
        return *(T*)allocator->allocateStatic(&this->ptr, uniformAlignment(), sizeof(T));
    }


};

template<typename T>
struct VertexAttribHelper : GAllocationHelper<T*> {
    VertexAttribBuffer vertexBuffer(GAllocator* allocator) const {
        return { allocator->buffer(this->ptr), VectorTraits<T>::length };
    }

    T* staticUpdate(int_t n, GAllocator* allocator) {
        return (T*)allocator->allocateStatic(&this->ptr, alignof(T), sizeof(T) * n);
    }

    T* allocateDynamic(int_t n, GAllocator* allocator) {
        return (T*)allocator->allocateDynamic(&this->ptr, alignof(T), sizeof(T) * n);
    }

    T* dynamicUpdate(GAllocator* allocator) {
        return (T*)allocator->mapping(this->ptr);
    }

    void dynamicUpdate(const ArrayView<const T>& data, GAllocator* allocator) {
        std::memcpy(dynamicUpdate(allocator), data.data(), sizeof(T) * data.size());
    }

    void staticUpdate(const ArrayView<const T>& data, GAllocator* allocator) {
        std::memcpy(staticUpdate(data.size(), allocator), data.data(), sizeof(T) * data.size());
    }


};
