#pragma once

#include "event/Callable.hpp"
#include "util/log.hpp"
#include <utility>

namespace Tag {
struct Resources {
    static constexpr char tag[] = "res";
    static constexpr bool verifyEnabled = true;
};
}

namespace detail {

template<typename T>
concept PtrReleasable = std::is_pointer_v<T>;

template<typename T>
struct IsMemberFunction {
    static constexpr bool value = false;
};
template<typename R, typename B, typename... Args>
struct IsMemberFunction<R (B::*)(Args...)> {
    static constexpr bool value = true;
};
template<typename T>
concept HasOperatorArrow = IsMemberFunction<decltype(&T::operator->)>::value;

template<typename T>
concept HasDestroy = std::is_same_v<decltype(&T::destroy), void (T::*)()>;

template<typename T>
struct DefaultDestroyCall;
template<HasDestroy T>
struct DefaultDestroyCall<T> {
    using type = evt::CallMember<&T::destroy>;
};

}

template<typename T, evt::BasicCallable<void, T&> DestroyCall = typename detail::DefaultDestroyCall<T>::type>
class Owned {
private:
    T value;

public:
    Owned()
        : value() { }
    Owned(const T& value)
        : value(value) { }
    Owned(const T& value, const DestroyCall&)
        : value(value) { }
    Owned(const Owned&) = delete;
    Owned(Owned&& other)
        : value(other.value) {
        other.value = {};
    }

    void set(const T& newValue) {
        destroy();
        value = newValue;
    }
    Owned& operator=(const Owned&) = delete;
    Owned& operator=(Owned&& other) {
        set(other.value);
        other.value = {};
        return *this;
    }
    Owned& operator=(const T& newValue) {
        set(newValue);
        return *this;
    }

    T take() {
        T ret = value;
        value = {};
        return ret;
    }

    void destroy() {
        DestroyCall {}(value);
    }

    const T* operator->() requires(!detail::HasOperatorArrow<T>) { return &value; }
    const T& operator->() requires detail::HasOperatorArrow<T> { return value; }
    const T& operator*() { return value; }
    const T& get() { return value; }

    ~Owned() {
        destroy();
    }
};
