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

    UniformBuffer uniformBuffer(GAllocator& allocator) {
        return { allocator.buffer(this->ptr), sizeof(T) };
    }

    T& allocateDynamic(GAllocator& allocator) {
        return *(T*)allocator.allocateDynamic(&this->ptr, uniformAlignment(), sizeof(T));
    }

    T& dynamicUpdate(GAllocator& allocator) {
        return *(T*)allocator.dynamicMapping(this->ptr);
    }

    T& staticUpdate(GAllocator& allocator) {
        return *(T*)allocator.allocateStatic(&this->ptr, uniformAlignment(), sizeof(T));
    }
};

template<typename T, int_t N>
struct VertexAttribHelper : GAllocationHelper<> {
    VertexAttribBuffer vertexBuffer(GAllocator& allocator) {
        return { allocator.buffer(ptr), VectorTraits<T>::length };
    }

    std::array<T, N>& staticUpdate(GAllocator& allocator) {
        return *(std::array<T, N>*)allocator.allocateStatic(&ptr, alignof(T), sizeof(T) * N);
    }
};
