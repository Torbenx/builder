#pragma once

template<typename T>
union uninitialized {
    T value;
    uninitialized() { }
    ~uninitialized() { value.~T(); }

    template<typename... Args>
    void initialize(Args&&... args) {
        new (&value) T(std::forward<Args>(args)...);
    }
};