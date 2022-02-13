#pragma once

#include <memory>

template<typename T>
union uninitialized {
    T value;
    constexpr uninitialized() { }
    constexpr ~uninitialized() { }

    template<typename... Args>
    constexpr void initialize(Args&&... args) {
        std::construct_at<T, Args...>(&value, std::forward<Args>(args)...);
    }

    constexpr void destroy() {
        std::destroy_at(&value);
    }
};
