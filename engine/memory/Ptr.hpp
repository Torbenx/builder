#pragma once

#include <type_traits>

template<typename uint_type>
struct BasicRawPtr {
    using uintptr_t = uint_type;
    uintptr_t m_addr = 0;
    constexpr uintptr_t addr() { return m_addr; }
    constexpr void setNull() { m_addr = 0; }
    constexpr bool isNull() { return m_addr == 0; }
};

template<typename RawPtr, typename T>
struct PtrBase : RawPtr { };

template<typename RawPtr, typename T = void>
struct Ptr : PtrBase<RawPtr, T> {
    using uintptr_t = typename RawPtr::uintptr_t;
    template<typename S = void>
    using BasicPtr = Ptr<BasicRawPtr<uintptr_t>, S>;
    using ptrdiff_t = std::make_signed_t<uintptr_t>;

    constexpr Ptr() { }
    constexpr Ptr(std::nullptr_t) { }
    constexpr Ptr(RawPtr rawPtr)
        : RawPtr(rawPtr) { }
    constexpr Ptr(uintptr_t addr)
        : RawPtr(addr) { }

    constexpr Ptr& operator=(std::nullptr_t) {
        this->setNull();
        return *this;
    }

    //constexpr bool operator==(const Ptr& other) const { return this->offset == other.offset; }
    constexpr explicit operator bool() { return !this->isNull(); }
    constexpr bool operator==(std::nullptr_t) { return this->isNull(); }
    constexpr bool operator!=(std::nullptr_t) { return !this->isNull(); }

    template<typename S>
    constexpr explicit operator Ptr<RawPtr, S> const { return { *(RawPtr*)this }; }
    constexpr operator Ptr<RawPtr>() const { return { *(RawPtr*)this }; }
    constexpr operator BasicPtr<T>() const { return { this->addr() }; }

    template<typename S>
    constexpr explicit operator BasicPtr<S>() const { return { this->addr() }; }
    constexpr operator BasicPtr<>() const { return { this->addr() }; }

    constexpr BasicPtr<T> operator+(ptrdiff_t i) { return { this->addr() + i * sizeof(T) }; }
    constexpr BasicPtr<T> operator-(ptrdiff_t i) { return { this->addr() - i * sizeof(T) }; }
};
template<typename RawPtr, typename T>
constexpr RawPtr::ptrdiff_t operator-(Ptr<RawPtr, T> a, Ptr<RawPtr, T> b) {
    return (a.addr() - b.addr()) / sizeof(T);
}

template<typename RawPtr, typename T>
requires std::is_class_v<T>
struct PtrBase<RawPtr, T> : RawPtr {
    using uintptr_t = typename RawPtr::uintptr_t;
    template<typename S = void>
    using BasicPtr = Ptr<BasicRawPtr<uintptr_t>, S>;
    using ptrdiff_t = std::make_signed_t<uintptr_t>;

    template<typename S>
    constexpr BasicPtr<S> operator->*(S T::*ptr) {
        T* t = nullptr;
        S* s = &(t->*ptr);
        return { this->addr() + (ptrdiff_t)((std::byte*)s - (std::byte*)t) };
    }
};

template<typename RawPtr, typename... Types>
struct PtrBase<RawPtr, std::tuple<Types...>> : PtrRaw {
    using uintptr_t = typename RawPtr::uintptr_t;
    template<typename S = void>
    using BasicPtr = Ptr<BasicRawPtr<uintptr_t>, S>;
    using ptrdiff_t = std::make_signed_t<uintptr_t>;

    template<typename S>
    constexpr BasicPtr<S> get() {
        std::tuple<Types...>* tuple = nullptr;
        S* s = &std::get<S>(*tuple);
        return { this->addr() + (ptrdiff_t)((std::byte*)s - (std::byte*)tuple) };
    }

    template<size_t I>
    constexpr BasicPtr<std::tuple_element_t<I, std::tuple<Types...>>> get() {
        std::tuple<Types...>* tuple = nullptr;
        std::tuple_element_t<I, std::tuple<Types...>>* s = &std::get<I>(*tuple);
        return { this->addr() + (ptrdiff_t)((std::byte*)s - (std::byte*)tuple) };
    }
};

template<typename RawPtr, typename T>
requires std::is_array_v<T>
struct PtrBase<RawPtr, T> : RawPtr {
    using uintptr_t = typename RawPtr::uintptr_t;
    template<typename S = void>
    using BasicPtr = Ptr<BasicRawPtr<uintptr_t>, S>;
    using ptrdiff_t = std::make_signed_t<uintptr_t>;

    constexpr auto get(int_t i) {
        using type = std::remove_extent_t<T>;
        return BasicPtr<type> { (uintptr_t)(this->addr() + i * sizeof(type)) };
    }
};

template<typename T = void>
using Ptr32 = Ptr<BasicRawPtr<uint32_t>, T>;
template<typename T = void>
using Ptr16 = Ptr<BasicRawPtr<uint16_t>, T>;
