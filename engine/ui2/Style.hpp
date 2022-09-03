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
    WindowsDIP,
    AndroidDIP,
    DIP = AndroidDIP,
    NoValue,
};
enum class Axis : uint8_t {
    Default = 0,
    X = Default,
    Y,
};
struct UnalignedTypedSize;
struct TypedSize {
    float value = 0.0f;
    SizeUnit unit : 24 = SizeUnit::Default;
    Axis axis : 8 = Axis::Default;
    static constexpr TypedSize noValue() { return { 0.0f, SizeUnit::NoValue, Axis::Default }; }
    static constexpr float noValueToFloat() { return NAN; }
    constexpr bool hasNoValue() const { return unit == SizeUnit::NoValue; }
    constexpr bool hasValue() const { return unit != SizeUnit::NoValue; }
    constexpr operator UnalignedTypedSize() const;
};
constexpr TypedSize operator-(const TypedSize& in) {
    return { -in.value, in.unit, in.axis };
}
struct UnalignedTypedSize {
    float value = 0.0f;
    SizeUnit unit = SizeUnit::Default;
    constexpr TypedSize inAxis(Axis axis) const { return { value, unit, axis }; }
    constexpr TypedSize inX() const { return inAxis(Axis::X); }
    constexpr TypedSize inY() const { return inAxis(Axis::Y); }
};
constexpr UnalignedTypedSize operator-(const UnalignedTypedSize& in) {
    return { -in.value, in.unit };
}
constexpr TypedSize::operator UnalignedTypedSize() const {
    return { value, unit };
}
namespace Size {
    constexpr UnalignedTypedSize fraction(float value) { return { value, SizeUnit::Fraction }; }
    constexpr UnalignedTypedSize pixels(float value) { return { value, SizeUnit::Pixels}; }
};
struct AxisMetrics {
    uint32_t m_extend = 0;
    float m_dpi = 0;

    int_t extend() const { return m_extend; }
    float dpi() const { return m_dpi; }
    float pixels(const UnalignedTypedSize& s) const {
        switch (s.unit) {
        case SizeUnit::Pixels:
            return s.value;
        case SizeUnit::Fraction:
            return s.value * extend();
        case SizeUnit::AndroidDIP:
            return s.value * dpi() / 160.0f;
        case SizeUnit::WindowsDIP:
            return s.value * dpi() / 96.0f;
        case SizeUnit::NoValue:
            return TypedSize::noValueToFloat();
        }
    }
};
struct ScreenMetrics {
    AxisMetrics xMetrics, yMetrics;
    AxisMetrics axis(Axis axis) const { return axis == Axis::X ? xMetrics : xMetrics; }
    int_t width() const { return xMetrics.extend(); }
    int_t height() const { return yMetrics.extend(); }
    float pixels(const TypedSize& s) const {
        return axis(s.axis).pixels(s);
    }
};

struct Rect {
    TypedSize left, right, bottom, top;
    static Rect noValues() {
        return { TypedSize::noValue(), TypedSize::noValue(), TypedSize::noValue(), TypedSize::noValue() };
    }
};
constexpr Rect operator-(const Rect& in) {
    return { -in.left, -in.right, -in.bottom, -in.top };
}
constexpr TypedSize applyLocalValue(const TypedSize& base, const TypedSize& localValue) {
    return localValue.hasValue() ? localValue : base;
}
constexpr Rect applyLocalValue(const Rect& base, const Rect& localValue) {
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