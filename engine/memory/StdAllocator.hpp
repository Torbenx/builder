#pragma once

#include "memory/Allocator.hpp"

class StdAllocator {
public:
    template<typename T = void>
    using Ptr = T*;
    using uintptr_t = std::uintptr_t;
    using ptrdiff_t = std::ptrdiff_t;

    void* allocate(size_t alignment, int_t size) {
        return ::operator new (size, std::align_val_t { alignment });
    }

    void free(void* ptr, size_t alignment) {
        ::operator delete (ptr, std::align_val_t { alignment });
    }
};
