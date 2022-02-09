#pragma once

#include "memory/Ptr.hpp"
#include "util/types.hpp"
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
struct AllocatorUtil {
private:
    Impl* impl() { return static_cast<Impl*>(this); }

public:
    struct Parameter { };
    struct Object { };

    using uintptr_t = uintptr_t_p;
    using RawPtr = RawPtr_p;
    template<typename T = void>
    using Ptr = ::Ptr<RawPtr, T>;

    template<typename T>
    T* operator[](Ptr<T> ptr) {
        return (T*)impl()->toAddress(ptr.addr());
    }
};

template<typename Impl, typename uintptr_t, typename RawPtr = BasicRawPtr<uintptr_t>>
struct AllocatorMethods {
private:
    template<typename T = void>
    using Ptr_ = ::Ptr<RawPtr, T>;

    Impl* impl() { return static_cast<Impl*>(this); }

public:
    template<typename T>
    Ptr_<T> allocate() {
        return (Ptr<T>)impl()->template allocate<alignof(T), sizeof(T)>();
    }

    template<typename T>
    Ptr_<T> allocate(int_t count) {
        return (Ptr<T>)impl()->template allocate<alignof(T)>(count * sizeof(T));
    }

    template<size_t alignment, int_t size>
    Ptr_<> allocate() {
        return impl()->template allocate<alignment>(size);
    }

    template<size_t alignment>
    Ptr_<> allocate(int_t size) {
        return impl()->allocateImpl(alignment, size);
    }

    template<typename T>
    Ptr_<T> allocatePowerOf2(int_t exponent) {
        if constexpr (std::has_single_bit(sizeof(T))) {
            constexpr int_t sizeExponent = std::countl_zero(sizeof(T));
            return impl()->template allocatePowerOf2<alignof(T)>(exponent + sizeExponent);
        } else
            return impl()->template allocate<T>((int_t)1 << exponent);
    }

    template<size_t alignment>
    Ptr_<> allocatePowerOf2(int_t exponent) {
        return impl()->template allocate<alignment>((int_t)1 << exponent);
    }

    template<typename T>
    void free(Ptr_<T> ptr) {
        impl()->template free<alignof(T), sizeof(T)>(ptr);
    }

    template<size_t alignment, int_t size>
    void free(Ptr_<> ptr) {
        impl()->template free<alignment>(ptr, size);
    }

    template<size_t alignment>
    void free(Ptr_<> ptr, int_t size) {
        impl()->freeImpl(alignment, ptr, size);
    }
};

template<typename Impl, typename uintptr_t_p, typename RawPtr_p = BasicRawPtr<uintptr_t_p>>
struct AllocatorBase : AllocatorUtil<Impl, uintptr_t_p, RawPtr_p>,
                       AllocatorMethods<Impl, uintptr_t_p, RawPtr_p> { };
