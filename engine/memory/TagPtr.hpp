#pragma once

#include "containers/Array.hpp"
#include "util/log.hpp"

namespace Tag {
struct Pointers {
    static constexpr char tag[] = "pointers";
    static constexpr bool verifyEnabled = true;
};
}

namespace stx {

template<typename T, int_t... Sizes>
struct TagPtr {
    static constexpr int_t TagCount = sizeof...(Sizes);
    uintptr_t storage = 0;
    static constexpr Array<int_t, TagCount> sizes { Sizes... };
    static constexpr auto positions = []() constexpr {
        Array<int_t, TagCount + 1> out;
        out[0] = 0;
        for (int_t i = 0; i < out.size(); i++)
            out[i + 1] = out[i] + sizes[i];
        return out;
    };
    static constexpr int_t tagSize() { return positions.back(); }
    static constexpr uintptr_t fullMask() {
        return ((uintptr_t)1 << tagSize()) - 1;
    }

    TagPtr() { }
    TagPtr(T* ptr) {
        setPtr(ptr);
    }
    TagPtr(const TagPtr<std::remove_const_t<T>, Sizes...>& ptr) requires std::is_const_v<T>
        : storage(ptr.storage) { }
    void setBase(uintptr_t val) {
        VERIFY(Pointers, (val & fullMask()) == 0);
        storage = val | (storage & fullMask());
    }
    void setValue(uintptr_t value) {
        setBase(value << tagSize());
    }
    void setPtr(T* ptr) {
        setBase((uintptr_t)ptr);
    }
    uintptr_t base() const { return storage & ~fullMask(); }
    uintptr_t value() const { return storage >> tagSize(); }
    T* ptr() const { return (T*)value(); }
    uintptr_t tag(int_t i) const { return (storage >> positions[i]) & (((uintptr_t)1 << sizes[i]) - 1); }
    void setTag(int_t i, uintptr_t tag) {
        uintptr_t mask = (((uintptr_t)1 << sizes[i]) - 1) << positions[i];
        tag <<= positions[i];
        storage &= ~mask;
        storage |= tag;
    }
    uintptr_t tag() const requires(TagCount == 1) {
        return storage & (((uintptr_t)1 << sizes.front()) - 1);
    }
    void setTag(uintptr_t tag) requires(TagCount == 1) {
        uintptr_t mask = (((uintptr_t)1 << sizes.front()) - 1);
        storage &= ~mask;
        storage |= tag;
    }
    T* operator->() const { return ptr(); }
    T& operator*() const { return *ptr(); }
};

}
