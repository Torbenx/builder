#pragma once

#include "util/types.hpp"
#include <type_traits>
#include <tuple>

template<typename Allocator>
concept Allocator32Bit = requires(Allocator allocator, int_t alignment, int_t size, uint32_t offset, void* address) {
    { allocator.allocate32(size) } -> std::same_as<uint32_t>; // allocate with __STDCPP_DEFAULT_NEW_ALIGNMENT__
    { allocator.allocate32(alignment, size) } -> std::same_as<uint32_t>;
    allocator.free32(offset);
    { allocator.address(offset) } -> std::same_as<void*>;
    { allocator.ownsAddress(address) } -> std::same_as<bool>;
    { allocator.offset(address) } -> std::same_as<uint32_t>;
};

struct Ptr32Raw {
    uint32_t offset = 0;
};

template<typename T>
struct Ptr32Base : Ptr32Raw { };

template<typename T = void>
struct Ptr32 : Ptr32Base<T> {

    Ptr32() { }
    Ptr32(std::nullptr_t) { }
    Ptr32(uint32_t offset)
        : Ptr32Base<T> { offset } { }

    Ptr32<T>& operator=(uint32_t offset) {
        this->offset = offset;
        return *this;
    }
    Ptr32<T>& operator=(std::nullptr_t) {
        this->offset = 0;
        return *this;
    }

    bool operator==(const Ptr32<>& other) const { return this->offset == other.offset; }
    explicit operator bool() { return this->offset != 0; }
    template<typename S>
    explicit operator Ptr32<S>() const { return { this->offset }; }

    Ptr32<T> operator+(int32_t i) { return { this->offset + i * sizeof(T) }; }
    Ptr32<T> operator-(int32_t i) { return { this->offset - i * sizeof(T) }; }
    Ptr32<T>& operator+=(int32_t i) {
        this->offset += i * sizeof(T);
        return *this;
    }
    Ptr32<T>& operator-=(int32_t i) {
        this->offset -= i * sizeof(T);
        return *this;
    }

    template<Allocator32Bit Allocator>
    T* operator[](Allocator allocator) {
        return (T*)allocator.address(this->offset);
    }
};
template<typename T>
int32_t operator-(Ptr32<T> a, Ptr32<T> b) {
    return (a.offset - b.offset) / sizeof(T);
}

template<typename T> requires std::is_class_v<T>
struct Ptr32Base<T> : Ptr32Raw {

    template<typename S>
    Ptr32<S> operator->*(S T::*ptr) requires std::is_class_v<T> {
        T* t = nullptr;
        S* s = &(t->*ptr);
        return { offset + (int32_t)((std::byte*)s - (std::byte*)t) };
    }
};

template<typename... Types>
struct Ptr32Base<std::tuple<Types...>> : Ptr32Raw {
    
    template<typename S>
    Ptr32<S> get() {
        std::tuple<Types...>* tuple;
        S* s = &std::get<S>(*tuple);
        return { offset + (int32_t)((std::byte*)s - (std::byte*)tuple) };
    }

    template<size_t I>
    Ptr32<std::tuple_element_t<I, std::tuple<Types...>>> get() {
        std::tuple<Types...>* tuple;
        std::tuple_element_t<I, std::tuple<Types...>>* s = &std::get<I>(*tuple);
        return { offset + (int32_t)((std::byte*)s - (std::byte*)tuple) };
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
