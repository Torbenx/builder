#pragma once

#include "util/log.hpp"
#include <utility>

namespace Tag {
struct Resources {
    static constexpr char tag[] = "resources";
    static constexpr bool verifyEnabled = true;
};
}

namespace detail {

template<typename T>
concept PtrReleasable = std::is_pointer_v<T>;

template<typename T>
concept SimpleReleasable = requires(T value) { value.release(); };

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

}

template<typename T>
concept Releasable = detail::PtrReleasable<T> || detail::SimpleReleasable<T>;

template<typename T>
requires Releasable<T>
class Owned {
private:
    T value;

public:
    Owned()
        : value() { }
    Owned(const T& value)
        : value(value) { }
    Owned(const Owned&) = delete;
    Owned(Owned&& other)
        : value(other.value) {
        other.value = {};
    }

    void set(const T& newValue) {
        release();
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
        T ret(value);
        value = {};
        return ret;
    }

    const T* operator->() requires(!detail::HasOperatorArrow<T>) { return &value; }
    const T& operator->() requires detail::HasOperatorArrow<T> { return value; }
    const T& operator*() { return value; }
    const T& get() { return value; }

    void release() requires detail::SimpleReleasable<T> {
        value.release();
    }

    void release() requires detail::PtrReleasable<T> {
        delete value;
        value = nullptr;
    }

    ~Owned() {
        release();
    }
};
