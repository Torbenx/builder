#pragma once

#include "memory/TagPtr.hpp"
#include "util/target.hpp"

namespace ui {
struct StyleKeyRecord {

};
}

DEFINE_SECTION(ui::StyleKeyRecord, ui_style_keys)

namespace ui {

template<typename T>
const void* genericApply(const void*, const void*);

template<typename Key, typename Type>
struct StyleKey {
    static constinit inline IN_SECTION(ui_style_keys) const StyleKeyRecord keyRecord = {};
    static int_t styleKeyIndex() { return &keyRecord - __begin_ui_style_keys; }
    using type = Type;
};

template<typename T>
struct StyleRef {
    stx::TagPtr<StyleRef, 1> ref;
};

struct StyleView {
    const void* const* data = nullptr;
    template<typename Item>
    const typename Item::type& get() { return *(const typename Item::type*)data[Item::styleKeyIndex()]; }
};

}