#pragma once

#include "util/types.hpp"
#include "memory/Ptr.hpp"
#include <bit>
#include <type_traits>

template<typename Allocator>
class AllocatorStorage {
public:
    AllocatorStorage() requires std::is_default_constructible_v<typename Allocator::Object> { }

    AllocatorStorage(typename Allocator::Object object)
        : object(object) { }

    Allocator getAllocator(typename Allocator::Parameter param) {
        return { object, param };
    }

private:
    [[no_unique_address]] typename Allocator::Object object;
};

template<typename Impl, typename uintptr_t_p, typename RawPtr_p = BasicRawPtr<uintptr_t_p>>
class AllocatorBase {
public:
    struct Parameter { };
    struct Object { };

    using uintptr_t = uintptr_t_p;
    using RawPtr = RawPtr_p;
    template<typename T = void>
    using Ptr = ::Ptr<RawPtr, T>;

    Impl* impl() { return static_cast<Impl*>(this); }

    template<typename T>
    Ptr<T> allocate() {
        return (Ptr<T>)impl()->template allocate<alignof(T), sizeof(T)>();
    }

    template<typename T>
    Ptr<T> allocate(int_t count) {
        return (Ptr<T>)impl()->allocate<alignof(T)>(count * sizeof(T));
    }

    template<size_t alignment, int_t size>
    Ptr<> allocate() {
        return impl()->allocate<alignment>(size);
    }

    template<size_t alignment>
    Ptr<> allocate(int_t size) {
        return impl()->allocateImpl(alignment, size);
    }

    template<typename T>
    Ptr<T> allocatePowerOf2(int_t exponent) {
        if constexpr (std::has_single_bit(sizeof(T))) {
            constexpr int_t sizeExponent = std::countl_zero(sizeof(T));
            return impl()->allocatePowerOf2<alignof(T)>(exponent + sizeExponent);
        } else
            return impl()->template allocate<T>((int_t)1 << exponent);
    }

    template<size_t alignment>
    Ptr<> allocatePowerOf2(int_t exponent) {
        return impl()->allocate<alignment>((int_t)1 << exponent);
    }

    template<typename T>
    void free(Ptr<T> ptr) {
        impl()->free<alignof(T), sizeof(T)>(ptr);
    }

    template<size_t alignment, int_t size>
    void free(Ptr<> ptr) {
        impl()->free<alignment>(ptr, size);
    }

    template<size_t alignment>
    void free(Ptr<> ptr, int_t size) {
        impl()->freeImpl(alignment, ptr, size);
    }
};
