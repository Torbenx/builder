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

DECLARE_RECORDED_CLASS(const ui::StyleKeyRecord, ui_style_keys)

namespace ui {

template<typename Key, typename T>
struct StyleKey {
    using Type = T;

    static_assert(alignof(wrapper<Type>) == alignof(Ref<Type>));
    static_assert(sizeof(wrapper<Type>) == sizeof(Ref<Type>));
    static constexpr StyleKeyRecord makeRecord() {
        return {
            .valueAlignment = alignof(wrapper<Type>),
            .valueSize = sizeof(wrapper<Type>),
            .resolveFunc = [](void* output, const void* input, const Resolver& res) {
                new (output) T { ((const Ref<T>*)input)->resolve(res) };
            },
        };
    }
    RECORDED_CLASS_IMPL(const ui::StyleKeyRecord, ui_style_keys, makeRecord);

    static uint32_t styleKey() {
        static_assert(std::is_base_of_v<StyleKey, Key>);
        return classIndex() + 1;
    }
};
template<typename Key>
const Ptr<typename Key::Type> ref() { return PtrInfo { .styleKey = Key::styleKey(), .byteOffset = 0 }; }

#define DECLARE_STYLE_KEY(name, type) \
    struct name : ui::StyleKey<name, type> { };
}