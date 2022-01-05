#pragma once

#include <bit>

template<typename Header, typename T>
struct alignas(T) TrailingArray {
    static int_t sizeInBytes(int_t size) {
        static_assert(std::is_base_of_v<TrailingArray, Header>);
        return size * sizeof(T) + sizeof(Header);
    }

    static int_t byteExponent(int_t exponent) {
        static_assert(std::has_single_bit(sizeof(T))); // FIXME
        constexpr int_t itemExponent = std::countl_zero(sizeof(T));
        return exponent + itemExponent;
    }

    static int_t remainingItems(int_t exponent) {
        static_assert(std::has_single_bit(sizeof(T))); // FIXME
        constexpr int_t headerSizeInItems = (sizeof(Header) + sizeof(T) - 1) / sizeof(T);
        return ((int_t)1 << exponent) - headerSizeInItems;
    }

    template<typename Allocator>
    static Allocator::Ptr<Header> allocate(int_t size, Allocator allocator) {
        // alignof(Header) takes into account alignof(T) since Header derives from this.
        return (Allocator::Ptr<Header>)allocator.allocate<alignof(Header)>(sizeInBytes(size));
    }

    template<typename Allocator>
    static Allocator::Ptr<Header> allocatePowerOf2(int_t exponent, Allocator allocator) {
        return (Allocator::Ptr<Header>)allocator.allocatePowerOf2<alignof(Header)>(byteExponent(exponent));
    }

    template<typename Allocator>
    static void free(Allocator::Ptr<Header> ptr, int_t size, Allocator allocator) {
        allocator.free<alignof(Header)>(ptr, size);
    }

    T* array() {
        static_assert(std::is_base_of_v<TrailingArray, Header>);
        return (T*)(static_cast<Header*>(this) + 1);
    }
};
