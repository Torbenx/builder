#pragma once

template<typename Header, typename T>
struct alignas(T) TrailingArray {
    static int_t sizeInBytes(int_t size) {
        static_assert(std::is_base_of_v<TrailingArray, Header>);
        return size * sizeof(T) + sizeof(Header);
    }

    template<typename Allocator>
    static Allocator::Ptr<Header> allocate(int_t size, Allocator allocator) {
        return (Allocator::Ptr<Header>)allocator.allocate<alignof(Header)>(sizeInBytes(size));
    }

    T* array() {
        static_assert(std::is_base_of_v<TrailingArray, Header>);
        return (T*)(static_cast<Header*>(this) + 1);
    }
};
