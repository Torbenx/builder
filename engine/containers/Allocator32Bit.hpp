#pragma once

#include "util/types.hpp"

template<typename Allocator>
concept Allocator32Bit = requires(Allocator allocator, int_t alignment, int_t size, uint32_t offset, void* address) {
    { allocator.allocate32(size) } -> std::same_as<uint32_t>; // allocate with __STDCPP_DEFAULT_NEW_ALIGNMENT__
    { allocator.allocate32(alignment, size) } -> std::same_as<uint32_t>;
    allocator.free32(offset);
    { allocator.address(offset) } -> std::same_as<void*>;
    { allocator.ownsAddress(address) } -> std::same_as<bool>;
    { allocator.offset(address) } -> std::same_as<uint32_t>;
};

template<typename T = void>
struct Ptr32 {
    uint32_t offset = 0;

    Ptr32() { }
    Ptr32(std::nullptr_t) { }
    Ptr32(uint32_t offset)
        : offset(offset) { }

    Ptr32<T>& operator=(uint32_t offset) {
        this->offset = offset;
        return *this;
    }
    Ptr32<T>& operator=(std::nullptr_t) {
        offset = 0;
        return *this;
    }

    bool operator==(const Ptr32<>& other) const { return offset == other.offset; }
    explicit operator bool() { return offset != 0; }
    operator Ptr32<>() const { return { offset }; }

    template<Allocator32Bit Allocator>
    T* get(Allocator allocator) {
        return (T*)allocator.address(offset);
    }
};

template<typename Callback>
requires requires(Callback callback) {
    { callback() } -> Allocator32Bit;
}
struct Allocator32BitWrapper {
    Callback callback;

    uint32_t allocate32(int_t size) { return callback().allocate32(size); }
    uint32_t allocate32(int_t alignment, int_t size) { return callback().allocate32(alignment, size); }
    void free32(uint32_t offset) { callback().free32(offset); }
    void* address(uint32_t offset) { return callback().address(offset); }
    bool ownsAddress(void* address) { return callback().ownsAddress(address); }
    uint32_t offset(void* address) { return callback().offset(address); }
};
