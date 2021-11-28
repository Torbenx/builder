#pragma once

#include "util/types.hpp"
#include <tuple>
#include <type_traits>
#include <concepts>

template<typename Map>
concept Mapping32Bit = requires(Map map, uint32_t offset, void* address) {
    { map.address(offset) } -> std::same_as<void*>;
    { map.offset(address) } -> std::same_as<uint32_t>;
};

template<typename Allocator>
concept Allocator32Bit = Mapping32Bit<Allocator> && requires(Allocator allocator, int_t alignment, int_t size, uint32_t offset, void* address) {
    { allocator.allocate32(alignment, size) } -> std::same_as<uint32_t>;
    allocator.free32(offset);
};

struct Ptr32Raw {
    uint32_t offset = 0;
};

template<typename T>
struct Ptr32Base : Ptr32Raw { };

template<typename T = void>
struct Ptr32 : Ptr32Base<T> {

    constexpr Ptr32() { }
    constexpr Ptr32(std::nullptr_t) { }
    constexpr Ptr32(uint32_t offset) { this->offset = offset; }

    constexpr Ptr32<T>& operator=(uint32_t offset) {
        this->offset = offset;
        return *this;
    }
    constexpr Ptr32<T>& operator=(std::nullptr_t) {
        this->offset = 0;
        return *this;
    }

    constexpr bool operator==(const Ptr32<T>& other) const { return this->offset == other.offset; }
    constexpr explicit operator bool() { return this->offset != 0; }
    template<typename S>
    constexpr explicit operator Ptr32<S>() const { return { this->offset }; }

    constexpr Ptr32<T> operator+(int32_t i) { return { this->offset + i * sizeof(T) }; }
    constexpr Ptr32<T> operator-(int32_t i) { return { this->offset - i * sizeof(T) }; }
    constexpr Ptr32<T>& operator+=(int32_t i) {
        this->offset += i * sizeof(T);
        return *this;
    }
    constexpr Ptr32<T>& operator-=(int32_t i) {
        this->offset -= i * sizeof(T);
        return *this;
    }

    template<Mapping32Bit Map>
    std::remove_extent_t<T>* operator[](Map&& map) {
        return (std::remove_extent_t<T>*)map.address(this->offset);
    }
};
template<typename T>
int32_t operator-(Ptr32<T> a, Ptr32<T> b) {
    return (a.offset - b.offset) / sizeof(T);
}

template<typename T>
requires std::is_class_v<T>
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
        std::tuple<Types...>* tuple = nullptr;
        S* s = &std::get<S>(*tuple);
        return { offset + (int32_t)((std::byte*)s - (std::byte*)tuple) };
    }

    template<size_t I>
    Ptr32<std::tuple_element_t<I, std::tuple<Types...>>> get() {
        std::tuple<Types...>* tuple = nullptr;
        std::tuple_element_t<I, std::tuple<Types...>>* s = &std::get<I>(*tuple);
        return { offset + (int32_t)((std::byte*)s - (std::byte*)tuple) };
    }
};

template<typename T>
requires std::is_array_v<T>
struct Ptr32Base<T> : Ptr32Raw {
    auto get(int_t i) {
        using type = std::remove_extent_t<T>;
        return Ptr32<type> { (uint32_t)(offset + i * sizeof(type)) };
    }
};

template<auto callback>
struct Allocator32BitWrapper {
    uint32_t allocate32(int_t alignment, int_t size) { return callback(this).allocate32(alignment, size); }
    void free32(uint32_t offset) { callback(this).free32(offset); }
    void* address(uint32_t offset) { return callback(this).address(offset); }
    uint32_t offset(void* address) { return callback(this).offset(address); }
};

template<Allocator32Bit Allocator>
void* operator new(size_t size, std::align_val_t alignment, Allocator&& allocator) {
    return allocator.address(allocator.allocate32((size_t)alignment, size));
}
template<Allocator32Bit Allocator>
void* operator new(size_t size, Allocator&& allocator) { return operator new(size, __STDCPP_DEFAULT_NEW_ALIGNMENT__, std::forward<Allocator>(allocator)); }

template<typename T, Allocator32Bit Allocator, typename... Args>
Ptr32<T> make32(Allocator&& allocator, Args&&... args) requires(!std::is_array_v<T>) {
    uint32_t offset = allocator.allocate32(alignof(T), sizeof(T));
    new (allocator.address(offset)) T(std::forward<Args>(args)...);
    return { offset };
}

template<typename T, Allocator32Bit Allocator>
auto make32(Allocator&& allocator, int_t size) requires std::is_array_v<T> {
    using type = std::remove_extent_t<T>;
    uint32_t offset = allocator.allocate32(alignof(type), size * sizeof(type));
    return Ptr32<T> { offset };
}
