#pragma once

#include "graphics/GAllocator.hpp"
#include "math/constants.hpp"

template<typename T>
class StaticUniformHelper {
public:
    GPtr ptr;

    UniformBuffer operator()(GAllocator& allocator) {
        return allocator.uniform(ptr, sizeof(T));
    }

    T& update(GAllocator& allocator) {
        return *(T*)allocator.staticUniform(&ptr, sizeof(T));
    }

    void release(GAllocator& allocator) {
        allocator.free(&ptr);
    }
};

template<typename T, int_t N>
class VertexAttribHelper {
public:
    GPtr ptr;

    VertexAttribBuffer operator()(GAllocator& allocator) {
        return allocator.attrib(ptr, VectorTraits<T>::length);
    }

    std::array<T, N>& update(GAllocator& allocator) {
        return *(std::array<T, N>*)allocator.staticAttrib(&ptr, sizeof(T) * N);
    }

    void release(GAllocator& allocator) {
        allocator.free(&ptr);
    }
};
