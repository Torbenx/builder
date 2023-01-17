#pragma once

#include "Ref.hpp"

namespace ui {

struct BigStyleTable {
    const void* const* data = nullptr;

    const void* fetch(PtrInfo ptr) const {
        const std::byte* base = (const std::byte*)data[ptr.styleKey];
        return base + ptr.byteOffset;
    }
    template<typename T>
    const T& get(Ptr<T> ptr) const {
        return *(const T*)fetch(ptr.info());
    }
    template<typename Key>
    const typename Key::Type& get() const {
        return *(const typename Key::Type*)data[Key::styleKey()];
    }
};

template<>
struct Ref<BigStyleTable> {
    static Ref<BigStyleTable> allocate();

    void* const* data = nullptr;

    void* fetch(PtrInfo ptr) const {
        std::byte* base = (std::byte*)data[ptr.styleKey];
        return base + ptr.byteOffset;
    }
    template<typename T>
    Ref<T>& get(Ptr<T> ptr) const {
        return *(Ref<T>*)fetch(ptr.info());
    }
    template<typename Key>
    Ref<typename Key::Type>& get() const {
        log("accessing {}", Key::styleKey());
        return *(Ref<typename Key::Type>*)data[Key::styleKey()];
    }

    BigStyleTable finalize() const;
};

}