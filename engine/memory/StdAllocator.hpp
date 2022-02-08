#pragma once

#include "memory/Allocator.hpp"

class StdAllocator {
public:
    template<typename T = void>
    using Ptr = T*;
    using uintptr_t = std::uintptr_t;

    StdAllocator(Object, Parameter) { }

    template<size_t alignment>
    void* allocateImpl(size_t alignment, int_t size) {
        return ::operator new (size, std::align_val_t { alignment });
    }

    void freeImpl(size_t alignment, void* ptr, int_t size) {
        ::operator delete (ptr, std::align_val_t { alignment });
    }

    template<typename T>
    T* operator[](T* ptr) {
        return ptr;
    }
};
