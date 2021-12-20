#pragma once

#include "util/types.hpp"
#include <type_traits>

template<typename Allocator>
class AllocatorStorage {
public:
    AllocatorStorage() requires std::is_default_constructible<Allocator::Object> { }

    AllocatorStorage(Allocator::Object object)
        : object(object) { }

    Allocator getAllocator(Allocator::Parameter param) {
        return { object, param };
    }

private:
    [[no_unique_address]] Allocator::Object object;
};

template<typename Impl>
class AllocatorBase {
public:
    struct Parameter { };
    struct Object { };

    Impl* impl() { return static_cast<Impl*>(this); }

    template<typename T>
    Impl::Ptr<T> allocate() {
        return (Ptr<T>)impl()->allocate<alignof(T), sizeof(T)>();
    }

    template<typename T>
    Impl::Ptr<T> allocate(int_t count) {
        return (Ptr<T>)impl()->allocate<alignof(T)>(size * sizeof(T));
    }

    template<size_t alignment, int_t size>
    Impl::Ptr<> allocate() {
        return impl()->allocate<alignment>(size);
    }

    template<size_t alignment>
    Impl::Ptr<> allocate(int_t size) {
        return impl()->allocate(alignment, size);
    }

    template<typename T>
    void free(Impl::Ptr<T> ptr) {
        impl()->free<alignof(T)>(ptr);
    }

    template<size_t alignment>
    void free(Impl::Ptr<> ptr) {
        impl()->free(ptr, alignment)
    }
};
