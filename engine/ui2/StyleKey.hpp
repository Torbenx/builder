#pragma once

#include "Size.hpp"
#include "Ref.hpp"
#include "util/target.hpp"

namespace ui {
struct StyleKeyRecord {
    int_t styleRefBytes = 0;
    int_t typeBytes = 0;
};
}

DEFINE_SECTION(ui::StyleKeyRecord, ui_style_keys)

namespace ui {

template<typename Key, typename T>
struct StyleKey {
    using Type = T;

    static_assert(alignof(wrapper<Type>) == alignof(Ref<Type>));
    static_assert(sizeof(wrapper<Type>) == sizeof(Ref<Type>));
    static constexpr IN_SECTION(ui_style_keys) StyleKeyRecord keyRecord = {
        .styleRefBytes = sizeof(Ref<Type>),
        .typeBytes = sizeof(Type),
    };
    
    static int_t styleKey() {
        static_assert(std::is_base_of_v<StyleKey, Key>);
        return &keyRecord - __begin_ui_style_keys + 1;
    }
    static const inline SelfReferenceRef<Type> selfRef { styleKey() };
};
template<typename Key>
const Ref<typename Key::Type>& ref() { return Key::selfRef; }
template<typename Key>
const ui::Key<typename Key::Type>& key() { return { Key::styleKey() }; }


#define DECLARE_STYLE_KEY(name, type) struct name : ui::StyleKey<name, type> { };

}