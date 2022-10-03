#pragma once

#include "Ref.hpp"
#include "Size.hpp"
#include "IntTypes.hpp"

namespace ui {

struct Range {
    Size m_low, m_high;
    constexpr Range() { }
    constexpr Range(const Size& value)
        : Range(value, value) { }
    constexpr Range(const Size& low, const Size& high)
        : m_low(low), m_high(high) { }
    constexpr Size& low() { return m_low; }
    constexpr const Size low() const { return m_low; }
    constexpr Size& high() { return m_high; }
    constexpr const Size high() const { return m_high; }
};

}

constexpr ui::Range operator-(const ui::Range& in) {
    return { -in.low(), -in.high() };
}

namespace ui {

struct Rect {
    Range m_x, m_y;
    constexpr Rect() { }
    constexpr Rect(const Size& value)
        : Rect(Range(value)) { }
    constexpr Rect(const Range& range)
        : Rect(range, range) { }
    constexpr Rect(const Range& x, const Range& y)
        : m_x(x), m_y(y) { }
    constexpr Range& x() { return m_x; }
    constexpr const Range x() const { return m_x; }
    constexpr Range& y() { return m_y; }
    constexpr const Range y() const { return m_y; }
    constexpr Range& axis(Axis axis) { return axis == Axis::X ? x() : y(); }
    constexpr const Range axis(Axis axis) const { return axis == Axis::X ? x() : y(); }
    constexpr Size& left() { return x().low(); }
    constexpr const Size left() const { return x().low(); }
    constexpr Size& right() { return x().high(); }
    constexpr const Size right() const { return x().high(); }
    constexpr Size& bottom() { return y().low(); }
    constexpr const Size bottom() const { return y().low(); }
    constexpr Size& top() { return y().high(); }
    constexpr const Size top() const { return y().high(); }
};

}

constexpr ui::Rect operator-(const ui::Rect& in) {
    return { -in.x(), -in.y() };
}

namespace ui {

template<>
struct Ref<Range> {
    Ref<Size> m_low;
    Ref<Size> m_high;
    Ref() { }
    Ref(const MakeSelfReference& selfRef)
        : m_low(selfRef), m_high(selfRef) { }
    Ref(const Range& range)
        : m_low(range.low()), m_high(range.high()) { }
    Ref(const Ref<Size>& value)
        : Ref(value, value) { }
    Ref(const Size& value)
        : Ref(Ref<Size>(value)) { }
    Ref(const Ref<Size>& low, const Ref<Size>& high)
        : m_low(low), m_high(high) { }
    Ref<Size>& low() { return m_low; }
    const Ref<Size> low() const { return m_low; }
    Ref<Size>& high() { return m_high; }
    const Ref<Size> high() const { return m_high; }
    Range value() const { return { low().value(), high().value() }; }
    Range resolve(const Resolver& resolver) const { return { low().resolve(resolver), high().resolve(resolver) }; }
    Range substitute(const Substituter& substituter, const Range& base) const { return { low().substitute(substituter, base.low()), high().substitute(substituter, base.high()) }; }
};
template<>
struct Ref<Rect> {
    Ref<Range> m_x;
    Ref<Range> m_y;
    Ref() { }
    Ref(const MakeSelfReference& selfRef)
        : m_x(selfRef), m_y(selfRef) { }
    Ref(const Rect& rect)
        : m_x(rect.x()), m_y(rect.y()) { }
    Ref(const Size& value)
        : Ref(Ref<Size>(value)) { }
    Ref(const Ref<Size>& value)
        : Ref(Ref<Range>(value)) { }
    Ref(const Range& range)
        : Ref(Ref<Range>(range)) { }
    Ref(const Ref<Range>& range)
        : Ref(range, range) { }
    Ref(const Ref<Range>& x, const Ref<Range>& y)
        : m_x(x), m_y(y) { }
    Ref<Range>& x() { return m_x; }
    const Ref<Range> x() const { return m_x; }
    Ref<Range>& y() { return m_y; }
    const Ref<Range> y() const { return m_y; }
    Ref<Range>& axis(Axis axis) { return axis == Axis::X ? x() : y(); }
    const Ref<Range> axis(Axis axis) const { return axis == Axis::X ? x() : y(); }
    Ref<Size>& left() { return x().low(); }
    const Ref<Size> left() const { return x().low(); }
    Ref<Size>& right() { return x().high(); }
    const Ref<Size> right() const { return x().high(); }
    Ref<Size>& bottom() { return y().low(); }
    const Ref<Size> bottom() const { return y().low(); }
    Ref<Size>& top() { return y().high(); }
    const Ref<Size> top() const { return y().high(); }
    Rect value() const { return { x().value(), y().value() }; }
    Rect resolve(const Resolver& resolver) const { return { x().resolve(resolver), y().resolve(resolver) }; }
    Rect substitute(const Substituter& substituter, const Rect& base) const { return { x().substitute(substituter, base.x()), y().substitute(substituter, base.y()) }; }
};

}