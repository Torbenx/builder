#pragma once

#include "containers/ArrayView.hpp"

template<class U, typename T>
class InstructureVariable {
public:
    InstructureVariable() { }

    template<class V, typename S>
    InstructureVariable(S V::*member) requires std::is_convertible_v<V*, U*> && std::is_convertible_v<S*, T*> {
        V* v = (V*)10000;
        U* u = (U*)v;
        S* s = &(v->*member);
        T* t = (T*)s;
        std::byte* base = (std::byte*)u;
        std::byte* target = (std::byte*)t;
        m_offset = target - base;
    }

    template<class V, class W>
    InstructureVariable(InstructureVariable<U, V> a, InstructureVariable<W, T> b) requires std::is_convertible_v<V*, W*> {
        V* v = (V*)10000;
        W* w = (W*)v;
        std::byte* oldTarget = (std::byte*)v;
        std::byte* oldBase = (std::byte*)w;
        m_offset = a.offset() + b.offset() + (oldBase - oldTarget);
    }

    template<class V, typename S>
    InstructureVariable(InstructureVariable<V, S> a) requires std::is_convertible_v<V*, U*> && std::is_convertible_v<S*, T*> {
        V* v = (V*)10000;
        U* u = (U*)v;
        std::byte* base = (std::byte*)u;
        std::byte* oldBase = (std::byte*)v;
        S* s = (S*)10000;
        T* t = (T*)s;
        std::byte* target = (std::byte*)t;
        std::byte* oldTarget = (std::byte*)s;
        m_offset = a.offset() + (target - oldTarget) - (base - oldBase);
    }

    template<class V, typename S>
    InstructureVariable<U, T>& operator=(InstructureVariable<V, S> a) requires std::is_convertible_v<V*, U*> && std::is_convertible_v<S*, T*> {
        *this = InstructureVariable<U, T>(a);
        return *this;
    }

    int_t offset() { return m_offset; }

    T& get(U* u) {
        std::byte* base = (std::byte*)u;
        return *(T*)(base + offset());
    }

private:
    int_t m_offset = 0;
};

template<class U, typename T>
InstructureVariable(T U::*) -> InstructureVariable<U, T>;

template<class U, typename T>
class InstructureArray {
public:
    InstructureArray() { }

    template<class V, int_t N>
    InstructureArray(T (V::*member)[N]) requires std::is_convertible_v<V*, U*>
        : m_size(N) {
        V* v = (V*)10000;
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
