#pragma once

#include "ArrayView.hpp"
#include <string_view>

namespace stx {

template<int_t N>
struct FixedString {
    char storage[N + 1] = {};

    constexpr FixedString() { }
    constexpr FixedString(const char (&str)[N + 1]) {
        std::copy(str, str + length() + 1, storage);
    }

    constexpr const char* data() const { return storage; }
    static constexpr int_t length() { return N; }
    static constexpr int_t size() { return N; }
    static constexpr bool empty() { return N == 0; }
    constexpr const char* begin() const { return data(); }
    constexpr const char* end() const { return begin() + size(); }
    constexpr operator std::string_view() const { return { data(), length() }; }
    constexpr operator const char*() const { return data(); }

    template<int_t M>
    constexpr bool operator==(const FixedString<M>& other) const {
        if constexpr (N != M) {
            return false;
        } else {
            for (int_t i = 0; i < N + 1; i++) {
                if (data()[i] != other.data()[i])
                    return false;
            }
            return true;
        }
    }

    template<int_t M>
    constexpr auto operator+(const FixedString<M>& other) const {
        FixedString<N + M> ret;
        std::copy(data(), data() + length(), ret.storage);
        std::copy(other.data(), other.data() + other.length(), ret.storage + length());
        return ret;
    }
    template<int_t M>
    constexpr auto operator+(const char (&str)[M]) {
        return *this + FixedString<M - 1>(str);
    }
};
template<int_t N>
FixedString(const char (&)[N]) -> FixedString<N - 1>;

}