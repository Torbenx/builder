#pragma once

#include "graphics/GAllocator.hpp"
#include "math/constants.hpp"
#include "util/Context.hpp"

template<typename T = void>
struct GAllocationHelper {
    TypedGPtr<T> ptr;

    void release(Context<GAllocator*> allocator = {}) {
        allocator->free(&ptr);
    }

    ~GAllocationHelper() {
        release();
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

    UniformBuffer uniformBuffer(Context<GAllocator*> allocator = {}) const {
        return { allocator->buffer(this->ptr), sizeof(T) };
    }

    T& allocateDynamic(Context<GAllocator*> allocator = {}) {
        return *(T*)allocator->allocateDynamic(&this->ptr, uniformAlignment(), sizeof(T));
    }

    T& dynamicUpdate(Context<GAllocator*> allocator = {}) {
        return *(T*)allocator->mapping(this->ptr);
    }

    T& staticUpdate(Context<GAllocator*> allocator = {}) {
        return *(T*)allocator->allocateStatic(&this->ptr, uniformAlignment(), sizeof(T));
    }
};

template<typename T>
struct VertexAttribHelper : GAllocationHelper<T*> {
    VertexAttribBuffer vertexBuffer(Context<GAllocator*> allocator = {}) const {
        return { allocator->buffer(this->ptr), VectorTraits<T>::length };
    }

    T* staticUpdate(int_t n, Context<GAllocator*> allocator = {}) {
        return (T*)allocator->allocateStatic(&this->ptr, alignof(T), sizeof(T) * n);
    }

    T* allocateDynamic(int_t n, Context<GAllocator*> allocator = {}) {
        return (T*)allocator->allocateDynamic(&this->ptr, alignof(T), sizeof(T) * n);
    }

    T* dynamicUpdate(Context<GAllocator*> allocator = {}) {
        return (T*)allocator->mapping(this->ptr);
    }

    void dynamicUpdate(const ArrayView<const T>& data, Context<GAllocator*> allocator = {}) {
        std::memcpy(dynamicUpdate(), data.data(), sizeof(T) * data.size());
    }

    void staticUpdate(const ArrayView<const T>& data, Context<GAllocator*> allocator = {}) {
        std::memcpy(staticUpdate(data.size()), data.data(), sizeof(T) * data.size());
    }
};
