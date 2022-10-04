#pragma once

#include "Ref.hpp"
#include "Size.hpp"
#include "util/target.hpp"

namespace ui {
struct StyleKeyRecord {
    int_t valueAlignment = 0;
    int_t valueSize = 0;
    void (*resolveFunc)(void* output, const void* input, const Resolver&);
};
}

DEFINE_SECTION(const ui::StyleKeyRecord, ui_style_keys)

namespace ui {

template<typename Key, typename T>
struct StyleKey {
    using Type = T;

    static_assert(alignof(wrapper<Type>) == alignof(Ref<Type>));
    static_assert(sizeof(wrapper<Type>) == sizeof(Ref<Type>));
    static constexpr IN_SECTION(ui_style_keys) StyleKeyRecord keyRecord = {
        .valueAlignment = alignof(wrapper<Type>),
        .valueSize = sizeof(wrapper<Type>),
        .resolveFunc = [](void* output, const void* input, const Resolver& res) {
            new (output) T { ((const Ref<T>*)input)->resolve(res) };
        },
    };

    static uint32_t styleKey() {
        static_assert(std::is_base_of_v<StyleKey, Key>);
        return &keyRecord - __begin_ui_style_keys + 1;
    }
    static const inline SelfReferenceRef<Type> selfRef { styleKey() };
};
template<typename Key>
const Ref<typename Key::Type>& ref() { return Key::selfRef; }
template<typename Key>
ui::Key<typename Key::Type> key() { return { Key::styleKey() }; }

#define DECLARE_STYLE_KEY(name, type) \
    struct name : ui::StyleKey<name, type> { };
}