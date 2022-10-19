#pragma once

#include "util/Callable.hpp"

namespace stx {

namespace detail {

    template<typename T>
    struct IsMemberFunction {
        static constexpr bool value = false;
    };
    template<typename R, typename B, typename... Args>
    struct IsMemberFunction<R (B::*)(Args...)> {
        static constexpr bool value = true;
    };

    template<typename T>
    struct DefaultDeallocateCall;
    template<typename T>
    requires requires { typename T::DestroyCall; }
    struct DefaultDeallocateCall<T> {
        using type = typename T::DestroyCall;
    };
    template<typename T>
    requires std::is_same_v<decltype(&T::deallocate), void (T::*)()>
    struct DefaultDeallocateCall<T> {
        using type = CallMember<&T::deallocate>;
    };

    template<typename T>
    struct TypeOrWrapper {
        class type {
        private:
            T value = {};

        protected:
            constexpr type() = default;
            constexpr type(const T& value)
                : value(value) { }

        public:
            constexpr operator T&() { return value; }
            constexpr T& operator->() requires std::is_pointer_v<T> { return value; }
            constexpr std::remove_pointer_t<T>& operator*() requires std::is_pointer_v<T> { return *value; }

            constexpr operator const T&() const { return value; }
            constexpr const T& operator->() const requires std::is_pointer_v<T> { return value; }
            
            constexpr type& operator=(const T& in) {
                value = in;
                return *this;
            }
        };
    };
    template<typename T>
    requires std::is_class_v<T>
    struct TypeOrWrapper<T> {
        using type = T;
    };
}

template<typename T>
struct DeallocateCall {
    using type = typename detail::DefaultDeallocateCall<T>::type;
};

template<typename T, BasicCallable<void, T&> DeallocateCall = typename DeallocateCall<T>::type>
class Owned : public detail::TypeOrWrapper<T>::type {
private:
    using Base = typename detail::TypeOrWrapper<T>::type;

public:
    constexpr Owned() = default;
    constexpr Owned(const T& value)
        : Base(value) { }
    constexpr Owned(const T& value, const DeallocateCall&)
        : Base(value) { }
    Owned(const Owned&) = delete;
    constexpr Owned(Owned&& other)
        : Base(other) {
        other.assign({});
    }

    constexpr void set(const T& newValue) {
        deallocate();
        assign(newValue);
    }
    Owned& operator=(const Owned&) = delete;
    constexpr Owned& operator=(Owned&& other) {
        set(other);
        other.assign({});
        return *this;
    }
    constexpr Owned& operator=(const T& newValue) {
        set(newValue);
        return *this;
    }

    constexpr T take() {
        T ret = (T&)*this;
        assign({});
        return ret;
    }

    constexpr void deallocate() {
        DeallocateCall {}((T&)*this);
    }

    ~Owned() {
        deallocate();
    }

private:
    void assign(const T& newValue) {
        *(Base*)this = newValue;
    }
};

}