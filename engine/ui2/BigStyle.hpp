#pragma once

#include "Ref.hpp"

namespace ui {

struct BigStyle {
    const void* const* data = nullptr;
    template<typename Key>
    const typename Key::Type& get() const {
        return *(const typename Key::Type*)data[Key::styleKey()];
    }
};
template<>
struct Ref<BigStyle> {
    static Ref<BigStyle> allocate();
    void* const* data = nullptr;
    void* fetch(RefInfo ref) const {
        std::byte* base = (std::byte*)data[ref.styleKey];
        return base + ref.byteOffset;
    }
    template<typename Key>
    Ref<typename Key::Type>& get() const {
        return *(Ref<typename Key::Type>*)data[Key::styleKey()];
    }
};

}