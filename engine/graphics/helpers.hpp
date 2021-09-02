#pragma once

#include "graphics/GAllocator.hpp"

template<typename T>
class StaticGData {
public:
    GPtr ptr;

    UniformBuffer operator()(GAllocator& allocator) {
        return allocator.buffer(ptr, sizeof(T));
    }

    T& update(GAllocator& allocator) {
        T* r = (T*)allocator.staticData(&ptr, sizeof(T));
        return *r;
    }

    void release(GAllocator& allocator) {
        allocator.free(&ptr);
    }
};
