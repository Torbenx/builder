#pragma once

#include "util/log.hpp"
#include <utility>

namespace Tag {
struct Resources {
    static constexpr char tag[] = "resources";
    static constexpr bool verifyEnabled = true;
};
}

template<typename T>
struct ReleaseMethod {
    template<typename... Args>
    static void performRelease(T& value, Args&&... args) {
        value.release(std::forward<Args>(args)...);
    }
};

template<typename T>
struct ReleaseMethod<T*> {
    static void performRelease(T* ptr) {
        delete ptr;
    }
};

template<typename T>
concept SimpleReleasable = requires(T value) { ReleaseMethod<T>::performRelease(value); };

template<typename T>
class Owned {
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
        T ret(std::move(value));
        value = {};
        return ret;
    }

    const T* operator->() { return &value; }
    const T& operator*() { return value; }

    template<typename... Args>
    void release(Args&&... args) {
        ReleaseMethod<T>::performRelease(value, std::forward<Args>(args)...);
        value = {};
    }

    ~Owned() {
        if constexpr (SimpleReleasable<T>) {
            release();
        } else {
            VERIFY(Resources, !value.needsRelease());
        }
    }

private:
    T value;
};