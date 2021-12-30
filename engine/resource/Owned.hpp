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
concept ParameterReleasable = requires(T value, typename T::ReleaseParameter param) { value.release(param); };

template<typename T>
concept PtrReleasable = std::is_pointer_v<T>;

template<typename T>
concept SimpleReleasable = requires(T value) { value.release(); };

template<typename T>
struct ParameterTraits {
    using type = struct { };
};
template<typename T>
requires ParameterReleasable<T>
struct ParameterTraits<T> {
    using type = typename T::ReleaseParameter;
};

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
concept Releasable = detail::ParameterReleasable<T> || detail::PtrReleasable<T> || detail::SimpleReleasable<T>;

template<typename T>
requires Releasable<T>
class Owned {
private:
    T value;
    using Parameter = typename detail::ParameterTraits<T>::type;

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

    void set(const T& newValue, Parameter param = {}) {
        release(param);
        value = newValue;
    }
    Owned& operator=(const Owned&) = delete;
    Owned& operator=(Owned&& other, Parameter param = {}) {
        set(other.value, param);
        other.value = {};
        return *this;
    }
    Owned& operator=(const T& newValue, Parameter param = {}) {
        set(newValue, param);
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

    void release(Parameter = {}) requires detail::SimpleReleasable<T> {
        value.release();
    }

    void release(Parameter = {}) requires detail::PtrReleasable<T> {
        delete value;
        value = nullptr;
    }

    void release(Parameter param = {}) requires detail::ParameterReleasable<T> {
        value.release(param);
    }

    ~Owned(Parameter param = {}) {
        release(param);
    }
};
