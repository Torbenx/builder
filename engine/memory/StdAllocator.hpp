#pragma once

#include "memory/Allocator.hpp"

struct StdAllocator : AllocatorBase<StdAllocator> {
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

    void* toAddress(uintptr_t addr) {
        return (void*)addr;
    }
};
