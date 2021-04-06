#pragma once

#include "util/log.hpp"
#include <memory>

template<class T>
class return_ptr {
public:
    return_ptr(T* ptr)
        : ptr(ptr) { }

    return_ptr()
        : ptr(nullptr) { }

    T&& move() {
        return std::move(*ptr);
    }

    void reset(T* new_ptr) {
        if (ptr)
            delete ptr;
        ptr = new_ptr;
    }

    T* release() {
        T* p = ptr;
        ptr = nullptr;
        return p;
    }

    std::unique_ptr<T> unique() {
        auto p = std::unique_ptr<T>();
        p.reset(release());
        return p;
    }

    std::shared_ptr<T> shared() {
        auto p = std::shared_ptr<T>();
        p.reset(release());
        return p;
    }

    return_ptr<T>& operator=(return_ptr<T> other) {
        reset(other.release());
        return *this;
    }

    ~return_ptr() {
        if (ptr)
            delete ptr;
    }

private:
    return_ptr(const return_ptr<T>& other);
    T* ptr;
};
