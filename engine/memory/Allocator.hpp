#pragma once

#include "util/types.hpp"
#include <bit>
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
    Impl::Ptr<T> allocatePowerOf2(int_t exponent) {
        if constexpr (std::has_single_bit(sizeof(T))) {
            constexpr int_t sizeExponent = std::countl_zero(sizeof(T));
            return allocatePowerOf2<alignof(T)>(exponent + sizeExponent);
        } else
            return allocate<T>((int_t)1 << exponent);
    }

    template<size_t alignment>
    Impl::Ptr<> allocatePowerOf2(int_t exponent) {
        return impl()->allocatePowerOfTwo(alignment, exponent);
    }

    Impl::Ptr<> allocatePowerOf2(size_t alignment, int_t exponent) {
        return impl()->allocate(alignment, (int_t)1 << exponent);
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
