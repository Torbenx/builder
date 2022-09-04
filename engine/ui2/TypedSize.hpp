#pragma once

#include "util/log.hpp"
#include <cmath>

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
    static constexpr TypedSize noValue() { return { 0.0f, SizeUnit::NoValue, Axis::Default }; }
    static constexpr float noValueToFloat() { return NAN; }
    float m_value = 0.0f;
    SizeUnit m_unit : 24 = SizeUnit::Default;
    Axis m_axis : 8 = Axis::Default;
    constexpr TypedSize() { }
    constexpr TypedSize(const UnalignedTypedSize&, Axis);
    constexpr TypedSize(float value, SizeUnit unit, Axis axis)
        : m_value(value), m_unit(unit), m_axis(axis) { }
    constexpr float value() const { return m_value; }
    constexpr SizeUnit unit() const { return m_unit; }
    constexpr Axis axis() const { return m_axis; }
    constexpr bool hasNoValue() const { return unit() == SizeUnit::NoValue; }
    constexpr bool hasValue() const { return unit() != SizeUnit::NoValue; }
    constexpr operator UnalignedTypedSize() const;
};
constexpr TypedSize operator-(const TypedSize& in) {
    return { -in.value(), in.unit(), in.axis() };
}
constexpr TypedSize applyLocalValue(const TypedSize& base, const TypedSize& localValue) {
    return localValue.hasValue() ? localValue : base;
}
struct UnalignedTypedSize {
    float m_value = 0.0f;
    SizeUnit m_unit = SizeUnit::Default;
    constexpr UnalignedTypedSize() { }
    constexpr UnalignedTypedSize(float value, SizeUnit unit)
        : m_value(value), m_unit(unit) { }
    constexpr float value() const { return m_value; }
    constexpr SizeUnit unit() const { return m_unit; }
    constexpr TypedSize in(Axis axis) const { return { value(), unit(), axis }; }
    constexpr TypedSize inX() const { return in(Axis::X); }
    constexpr TypedSize inY() const { return in(Axis::Y); }
};
constexpr UnalignedTypedSize operator-(const UnalignedTypedSize& in) {
    return { -in.value(), in.unit() };
}
constexpr TypedSize::TypedSize(const UnalignedTypedSize& s, Axis axis)
    : TypedSize(s.value(), s.unit(), axis) { }
constexpr TypedSize::operator UnalignedTypedSize() const {
    return { value(), unit() };
}

struct Range {
    static Range noValue() { return Range { TypedSize::noValue() }; }
    TypedSize m_low, m_high;
    constexpr Range() { }
    explicit constexpr Range(const TypedSize& value)
        : Range(value, value) { }
    constexpr Range(const TypedSize& low, const TypedSize& high)
        : m_low(low), m_high(high) { }
    constexpr TypedSize& low() { return m_low; }
    constexpr const TypedSize& low() const { return m_low; }
    constexpr TypedSize& high() { return m_high; }
    constexpr const TypedSize& high() const { return m_high; }
};
constexpr Range operator-(const Range& in) {
    return { -in.low(), -in.high() };
}
constexpr Range applyLocalValue(const Range& base, const Range& localValue) {
    return {
        applyLocalValue(base.low(), localValue.low()),
        applyLocalValue(base.high(), localValue.high()),
    };
}

template<typename T, Axis defaultAxis>
struct DefaultAxis;

template<Axis defaultAxis>
struct DefaultAxis<TypedSize, defaultAxis> : TypedSize {
    using TypedSize::TypedSize;
    constexpr DefaultAxis(const UnalignedTypedSize& s)
        : TypedSize(s.in(defaultAxis)) { }
};
template<Axis defaultAxis>
struct DefaultAxis<Range, defaultAxis> : Range {
    using Range::Range;
    constexpr DefaultAxis(const Range& range)
        : Range(range) { }
    constexpr DefaultAxis(const UnalignedTypedSize& s)
        : Range(s.in(defaultAxis)) { }
    constexpr DefaultAxis(const UnalignedTypedSize& low, const UnalignedTypedSize& high)
        : Range(low.in(defaultAxis), high.in(defaultAxis)) { }
};

struct Rect {
    static Rect noValues() { return Rect { Range::noValue() }; }
    Range m_x, m_y;
    constexpr Rect() { }
    explicit constexpr Rect(TypedSize value)
        : Rect(Range(value)) { }
    explicit constexpr Rect(Range range)
        : Rect(range, range) { }
    constexpr Rect(DefaultAxis<Range, Axis::X> x, DefaultAxis<Range, Axis::Y> y)
        : m_x(x), m_y(y) { }
    constexpr Range& x() { return m_x; }
    constexpr const Range& x() const { return m_x; }
    constexpr Range& y() { return m_y; }
    constexpr const Range& y() const { return m_y; }
    constexpr Range& axis(Axis axis) { return axis == Axis::X ? x() : y(); }
    constexpr const Range& axis(Axis axis) const { return axis == Axis::X ? x() : y(); }
    constexpr TypedSize& left() { return x().low(); }
    constexpr const TypedSize& left() const { return x().low(); }
    constexpr TypedSize& right() { return x().high(); }
    constexpr const TypedSize& right() const { return x().high(); }
    constexpr TypedSize& bottom() { return y().low(); }
    constexpr const TypedSize& bottom() const { return y().low(); }
    constexpr TypedSize& top() { return y().high(); }
    constexpr const TypedSize& top() const { return y().high(); }
};
constexpr Rect operator-(const Rect& in) {
    return { -in.x(), -in.y() };
}
constexpr Rect applyLocalValue(const Rect& base, const Rect& localValue) {
    return {
        applyLocalValue(base.x(), localValue.x()),
        applyLocalValue(base.y(), localValue.y()),
    };
}

namespace literals {
    constexpr UnalignedTypedSize operator"" _px(unsigned long long value) {
        return { (float)value, SizeUnit::Pixels };
    }
    constexpr UnalignedTypedSize operator"" _px(long double value) {
        return { (float)value, SizeUnit::Pixels };
    }
    constexpr UnalignedTypedSize operator"" _frac(unsigned long long value) {
        return { (float)value, SizeUnit::Fraction };
    }
    constexpr UnalignedTypedSize operator"" _frac(long double value) {
        return { (float)value, SizeUnit::Fraction };
    }
}

struct AxisMetrics {
    uint32_t m_extend = 0;
    float m_dpi = 0;

    constexpr uint32_t& extend() { return m_extend; }
    constexpr const uint32_t& extend() const { return m_extend; }
    constexpr float& dpi() { return m_dpi; }
    constexpr const float& dpi() const { return m_dpi; }
    float pixels(const UnalignedTypedSize& s) const {
        switch (s.unit()) {
        case SizeUnit::Pixels:
            return s.value();
        case SizeUnit::Fraction:
            return s.value() * extend();
        case SizeUnit::AndroidDIP:
            return s.value() * dpi() / 160.0f;
        case SizeUnit::WindowsDIP:
            return s.value() * dpi() / 96.0f;
        case SizeUnit::NoValue:
            return TypedSize::noValueToFloat();
        }
    }
};
struct ScreenMetrics {
    AxisMetrics m_x, m_y;
    constexpr AxisMetrics& x() { return m_x; }
    constexpr const AxisMetrics& x() const { return m_x; }
    constexpr AxisMetrics& y() { return m_y; }
    constexpr const AxisMetrics& y() const { return m_y; }
    constexpr AxisMetrics& axis(Axis axis) { return axis == Axis::X ? x() : y(); }
    constexpr const AxisMetrics& axis(Axis axis) const { return axis == Axis::X ? x() : y(); }
    constexpr uint32_t& width() { return x().extend(); }
    constexpr const uint32_t& width() const { return x().extend(); }
    constexpr uint32_t& height() { return y().extend(); }
    constexpr const uint32_t& height() const { return y().extend(); }
    float pixels(const TypedSize& s) const { return axis(s.axis()).pixels(s); }
};

}