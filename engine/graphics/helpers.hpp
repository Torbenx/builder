#pragma once

#include "graphics/GAllocator.hpp"
#include "math/constants.hpp"

template<typename T = void>
struct GAllocationHelper {
    TypedGPtr<T> ptr;

    void release([[context]] GAllocator* allocator) {
        allocator->free(&ptr);
    }

    ~GAllocationHelper([[context]] GAllocator* allocator) {
        VERIFY(Graphics, !ptr || allocator);
        release(allocator);
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
    UniformBuffer uniformBuffer([[context]] GAllocator* allocator) const {
        return { allocator->buffer(this->ptr), sizeof(T) };
    }

    T& allocateDynamic([[context]] GAllocator* allocator) {
        return *(T*)allocator->allocateDynamic(&this->ptr, uniformAlignment(), sizeof(T));
    }

    T& dynamicUpdate([[context]] GAllocator* allocator) {
        return *(T*)allocator->mapping(this->ptr);
    }

    T& staticUpdate([[context]] GAllocator* allocator) {
        return *(T*)allocator->allocateStatic(&this->ptr, uniformAlignment(), sizeof(T));
    }

    ~UniformHelper([[context]] GAllocator* allocator) { }
};

template<typename T>
struct VertexAttribHelper : GAllocationHelper<T*> {
    VertexAttribBuffer vertexBuffer([[context]] GAllocator* allocator) const {
        return { allocator->buffer(this->ptr), VectorTraits<T>::length };
    }

    T* staticUpdate(int_t n, [[context]] GAllocator* allocator) {
        return (T*)allocator->allocateStatic(&this->ptr, alignof(T), sizeof(T) * n);
    }

    T* allocateDynamic(int_t n, [[context]] GAllocator* allocator) {
        return (T*)allocator->allocateDynamic(&this->ptr, alignof(T), sizeof(T) * n);
    }

    T* dynamicUpdate([[context]] GAllocator* allocator) {
        return (T*)allocator->mapping(this->ptr);
    }

    void dynamicUpdate(const ArrayView<const T>& data, [[context]] GAllocator* allocator) {
        std::memcpy(dynamicUpdate(allocator), data.data(), sizeof(T) * data.size());
    }

    void staticUpdate(const ArrayView<const T>& data, [[context]] GAllocator* allocator) {
        std::memcpy(staticUpdate(data.size(), allocator), data.data(), sizeof(T) * data.size());
    }

    ~VertexAttribHelper([[context]] GAllocator* allocator) { }
};
