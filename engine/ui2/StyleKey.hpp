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
    static constexpr IN_SECTION(ui_style_keys) StyleKeyRecord keyRecord = {
        .styleRefBytes = sizeof(Ref<Type>),
        .typeBytes = sizeof(Type),
    };
    static int_t styleKey() {
        static_assert(std::is_base_of_v<StyleKey, Key>);
        return &keyRecord - __begin_ui_style_keys;
    }
    static const inline Ref<Type> selfRef = Ref<Type>(MakeSelfReference(styleKey()));
};
template<typename Key>
const auto& ref() { return Key::selfRef; }

}