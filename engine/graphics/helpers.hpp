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

template<typename T>
struct StaticUniformHelper : GAllocationHelper<T> {
    UniformBuffer operator()(GAllocator& allocator) {
        return allocator.uniform(this->ptr, sizeof(T));
    }

    T& update(GAllocator& allocator) {
        return *(T*)allocator.staticUniform(&this->ptr, sizeof(T));
    }
};

template<typename T, int_t N>
struct VertexAttribHelper : GAllocationHelper<> {
    VertexAttribBuffer operator()(GAllocator& allocator) {
        return allocator.attrib(ptr, VectorTraits<T>::length);
    }

    std::array<T, N>& update(GAllocator& allocator) {
        return *(std::array<T, N>*)allocator.staticAttrib(&ptr, sizeof(T) * N);
    }
};
