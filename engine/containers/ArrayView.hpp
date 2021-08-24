#pragma once

#include "util/types.hpp"
#include <concepts>
#include <type_traits>

template<typename T>
class ArrayView {
public:
    ArrayView() { }

    template<int_t N>
    ArrayView(T (&data)[N])
        : m_data(data), m_size(N) { }

    ArrayView(T* data, int_t size)
        : m_data(data), m_size(size) { }

    T* data() const { return m_data; }
    int_t size() const { return m_size; }
    T& operator[](int_t i) const { return data()[i]; }

private:
    T* m_data = nullptr;
    int_t m_size = 0;
};

template<class U, typename T>
class InstructureVariable {
public:
    template<class V, typename S>
    InstructureVariable(S V::*member) requires std::is_convertible_v<V*, U*> && std::is_convertible_v<S*, T*> {
        V* v = nullptr;
        U* u = (U*)v;
        S* s = &(v->*member);
        T* t = (T*)s;
        std::byte* base = (std::byte*)u;
        std::byte* target = (std::byte*)t;
        m_offset = target - base;
    }

    int_t offset() { return m_offset; }

    T& get(U* u) {
        std::byte* base = (std::byte*)u;
        return *(T*)(base + offset());
    }

private:
    int_t m_offset;
};

template<class U, typename T>
class InstructureArray {
public:
    InstructureArray() { }

    template<class V, int_t N>
    InstructureArray(T (V::*member)[N]) requires std::is_convertible_v<V*, U*>
        : m_size(N) {
        V* v = nullptr;
        U* u = (U*)v;
        std::byte* base = (std::byte*)u;
        std::byte* target = (std::byte*)&(v->*member);
        m_offset = target - base;
    }

    ArrayView<T> view(U* u) {
        std::byte* base = (std::byte*)u;
        T* data = (T*)(base + offset());
        return { data, size() };
    }

    int_t offset() { return m_offset; }
    int_t size() { return m_size; }

private:
    int_t m_offset = 0;
    int_t m_size = 0;
};
