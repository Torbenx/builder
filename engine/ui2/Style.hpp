#pragma once

#include "util/target.hpp"
#include <cmath>

DEFINE_LINK_COUNTER(ui_styles)

namespace Tag {
struct Ui {
    static constexpr char tag[] = "ui";
    static constexpr bool verifyEnabled = true;
};
}

namespace ui {

enum class SizeUnit : uint32_t {
    Default = 0,
    Pixels = Default,
    Fraction,
    WinDIP,
    AndroidDIP,
    DIP = AndroidDIP,
};
enum class Axis : uint8_t {
    Default = 0,
    Horizontal = Default,
    Vertical,
};
struct TypedSize {
    float value = 0.0f;
    SizeUnit unit : 24 = SizeUnit::Default;
    Axis axis : 8 = Axis::Default;
    static constexpr float NO_VALUE_MAGIC = NAN;
    static constexpr TypedSize noValue() { return { NO_VALUE_MAGIC }; }
    bool operator==(const TypedSize& other) const {
        return value == other.value && unit == other.unit && axis == other.axis;
    }
    bool hasNoValue() const { return *this == noValue(); }
    bool hasValue() const { return *this != noValue(); }
};
TypedSize operator-(const TypedSize& in) {
    return { -in.value, in.unit, in.axis };
}
struct Rect {
    TypedSize left, right, bottom, top;
};
Rect operator-(const Rect& in) {
    return { -in.left, -in.right, -in.bottom, -in.top };
}
TypedSize applyLocalValue(const TypedSize& base, const TypedSize& localValue) {
    return localValue.hasValue() ? localValue : base;
}
Rect applyLocalValue(const Rect& base, const Rect& localValue) {
    return {
        applyLocalValue(base.left, localValue.left),
        applyLocalValue(base.right, localValue.right),
        applyLocalValue(base.bottom, localValue.bottom),
        applyLocalValue(base.top, localValue.top),
    };
}

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