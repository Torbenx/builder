#pragma once

#include <type_traits>

template<typename uint_type>
struct BasicRawPtr {
    using uintptr_t = uint_type;
    uintptr_t m_addr = 0;

    BasicRawPtr() { }
    BasicRawPtr(uintptr_t addr)
        : m_addr(addr) { }
    constexpr uintptr_t addr() const { return m_addr; }
    constexpr void setNull() { m_addr = 0; }
    constexpr bool isNull() const { return m_addr == 0; }
};

template<typename RawPtr, typename T>
struct PtrBase : RawPtr {
    using RawPtr::RawPtr;
    PtrBase(const RawPtr& rawPtr)
        : RawPtr(rawPtr) { }
};

template<typename RawPtr, typename T = void>
struct Ptr : PtrBase<RawPtr, T> {
    using uintptr_t = typename RawPtr::uintptr_t;
    template<typename S = void>
    using BasicPtr = Ptr<BasicRawPtr<uintptr_t>, S>;
    static constexpr bool IS_BASIC = std::is_same_v<RawPtr, BasicRawPtr<uintptr_t>>;
    using ptrdiff_t = std::make_signed_t<uintptr_t>;

    constexpr Ptr() { }
    constexpr Ptr(std::nullptr_t) { }
    constexpr Ptr(RawPtr rawPtr)
        : PtrBase<RawPtr, T>(rawPtr) { }
    constexpr Ptr(uintptr_t addr)
        : PtrBase<RawPtr, T>(addr) { }

    constexpr Ptr& operator=(std::nullptr_t) {
        this->setNull();
        return *this;
    }

    constexpr bool operator==(const Ptr& other) const { return this->addr() == other.addr(); }
    constexpr explicit operator bool() { return !this->isNull(); }
    constexpr bool operator==(std::nullptr_t) { return this->isNull(); }
    constexpr bool operator!=(std::nullptr_t) { return !this->isNull(); }

    template<typename S>
    constexpr explicit operator Ptr<RawPtr, S>() const requires(!std::is_void_v<S>) { return { *(RawPtr*)this }; }
    constexpr operator Ptr<RawPtr>() const { return { *(RawPtr*)this }; }

    template<typename S>
    constexpr explicit operator BasicPtr<S>() const requires(!std::is_void_v<S> && !IS_BASIC) { return { this->addr() }; }
    constexpr operator BasicPtr<T>() const requires(!std::is_void_v<T> && !IS_BASIC) { return { this->addr() }; }
    constexpr operator BasicPtr<>() const requires(!IS_BASIC) { return { this->addr() }; }

    constexpr BasicPtr<T> operator+(ptrdiff_t i) { return { this->addr() + i * sizeof(T) }; }
    constexpr BasicPtr<T> operator-(ptrdiff_t i) { return { this->addr() - i * sizeof(T) }; }
};
template<typename RawPtr, typename T>
constexpr typename Ptr<RawPtr, T>::ptrdiff_t operator-(Ptr<RawPtr, T> a, Ptr<RawPtr, T> b) {
    return (a.addr() - b.addr()) / sizeof(T);
}

template<typename RawPtr, typename T>
requires std::is_class_v<T>
struct PtrBase<RawPtr, T> : RawPtr {
    using uintptr_t = typename RawPtr::uintptr_t;
    template<typename S = void>
    using BasicPtr = Ptr<BasicRawPtr<uintptr_t>, S>;
    using ptrdiff_t = std::make_signed_t<uintptr_t>;

    using RawPtr::RawPtr;
    PtrBase(const RawPtr& rawPtr)
        : RawPtr(rawPtr) { }

    template<typename S>
    constexpr BasicPtr<S> operator->*(S T::*ptr) {
        T* t = nullptr;
        S* s = &(t->*ptr);
        return { this->addr() + (ptrdiff_t)((std::byte*)s - (std::byte*)t) };
    }
};

template<typename RawPtr, typename... Types>
struct PtrBase<RawPtr, std::tuple<Types...>> : RawPtr {
    using uintptr_t = typename RawPtr::uintptr_t;
    template<typename S = void>
    using BasicPtr = Ptr<BasicRawPtr<uintptr_t>, S>;
    using ptrdiff_t = std::make_signed_t<uintptr_t>;

    using RawPtr::RawPtr;
    PtrBase(const RawPtr& rawPtr)
        : RawPtr(rawPtr) { }

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

    using RawPtr::RawPtr;
    PtrBase(const RawPtr& rawPtr)
        : RawPtr(rawPtr) { }

    constexpr auto get(int_t i) {
        using type = std::remove_extent_t<T>;
        return BasicPtr<type> { (uintptr_t)(this->addr() + i * sizeof(type)) };
    }
};

/*template<typename T = void>
using Ptr32 = Ptr<BasicRawPtr<uint32_t>, T>;
template<typename T = void>
using Ptr16 = Ptr<BasicRawPtr<uint16_t>, T>;*/
