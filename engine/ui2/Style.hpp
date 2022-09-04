#pragma once

#include "util/target.hpp"

DEFINE_LINK_COUNTER(ui_styles)

namespace ui {

template<typename Derived, typename T>
struct StyleItem {
    LINK_COUNTER_IMPL(ui_styles);
    using type = T;
};
struct StyleView {
    const void* const* data = nullptr;
    template<typename Item>
    const typename Item::type& get() { return *(const typename Item::type*)data[Item::linkCounter()]; }
};

}