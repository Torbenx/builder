#pragma once

#include "Ref.hpp"
#include "Size.hpp"

namespace ui {

enum class Axis {
    Default = 0,
    X = Default,
    Y,
};

struct IntVec {
    int m_x = 0, m_y = 0;
    constexpr IntVec() { }
    constexpr IntVec(int xy)
        : IntVec(xy, xy) { }
    constexpr IntVec(int x, int y)
        : m_x(x), m_y(y) { }
    constexpr int& x() { return m_x; }
    constexpr int x() const { return m_x; }
    constexpr int& y() { return m_y; }
    constexpr int y() const { return m_y; }
    constexpr bool operator==(const IntVec&) const = default;
};

}

constexpr ui::IntVec operator+(const ui::IntVec& l, const ui::IntVec& r) {
    return { l.x() + r.x(), l.y() + r.y() };
}
constexpr ui::IntVec operator-(const ui::IntVec& l, const ui::IntVec& r) {
    return { l.x() - r.x(), l.y() - r.y() };
}
constexpr ui::IntVec operator-(const ui::IntVec& in) {
    return { -in.x(), -in.y() };
}
constexpr ui::IntVec operator*(const ui::IntVec& l, int r) {
    return { l.x() * r, l.y() * r };
}
constexpr ui::IntVec operator*(int l, const ui::IntVec& r) {
    return { l * r.x(), l * r.y() };
}
constexpr ui::IntVec operator/(const ui::IntVec& l, int r) {
    return { l.x() / r, l.y() / r };
}

namespace ui {

#define INT_VEC_ALIAS(name)               \
    struct name : IntVec {                \
        using IntVec::IntVec;             \
        constexpr name(const IntVec& vec) \
            : IntVec(vec) { }             \
    };

INT_VEC_ALIAS(Dimensions);
INT_VEC_ALIAS(BottomLeft);
INT_VEC_ALIAS(BottomRight);
INT_VEC_ALIAS(TopLeft);
INT_VEC_ALIAS(TopRight);

struct IntRange {
    int m_low = 0, m_high = 0;
    constexpr IntRange() { }
    constexpr IntRange(int value)
        : IntRange(value, value) { }
    constexpr IntRange(int low, int high)
        : m_low(low), m_high(high) { }
    constexpr int& low() { return m_low; }
    constexpr int low() const { return m_low; }
    constexpr int& high() { return m_high; }
    constexpr int high() const { return m_high; }
    constexpr int extend() const { return high() - low() + 1; }
    constexpr bool operator==(const IntRange&) const = default;
};

}

constexpr ui::IntRange operator-(const ui::IntRange& in) {
    return { -in.low(), -in.high() };
}

namespace ui {

struct IntRect {
    IntRange m_x, m_y;
    constexpr IntRect() { }
    constexpr IntRect(const int& value)
        : IntRect(IntRange(value)) { }
    constexpr IntRect(const IntRange& range)
        : IntRect(range, range) { }
    constexpr IntRect(const IntRange& x, const IntRange& y)
        : m_x(x), m_y(y) { }

    constexpr IntRect(BottomLeft bl, TopRight tr)
        : m_x { bl.x(), tr.x() }, m_y { bl.y(), tr.y() } { }
    constexpr IntRect(TopRight tr, BottomLeft bl)
        : m_x { bl.x(), tr.x() }, m_y { bl.y(), tr.y() } { }
    constexpr IntRect(BottomRight br, TopLeft tl)
        : m_x { tl.x(), br.x() }, m_y { br.y(), tl.y() } { }
    constexpr IntRect(TopLeft tl, BottomRight br)
        : m_x { tl.x(), br.x() }, m_y { br.y(), tl.y() } { }

    constexpr IntRect(BottomLeft bl, Dimensions dim)
        : IntRect(bl, TopRight(bl + (dim - 1))) { }
    constexpr IntRect(TopRight tr, Dimensions dim)
        : IntRect(tr, BottomLeft(tr - (dim - 1))) { }
    constexpr IntRect(BottomRight br, Dimensions dim)
        : IntRect(br, TopLeft(br + IntVec { -(dim.x() - 1), (dim.y() - 1) })) { }
    constexpr IntRect(TopLeft tl, Dimensions dim)
        : IntRect(tl, BottomRight(tl + IntVec { (dim.x() - 1), -(dim.y() - 1) })) { }
    constexpr IntRect(Dimensions dim)
        : IntRect(BottomLeft {}, dim) { }

    constexpr IntRange& x() { return m_x; }
    constexpr const IntRange x() const { return m_x; }
    constexpr IntRange& y() { return m_y; }
    constexpr const IntRange y() const { return m_y; }
    constexpr IntRange& axis(Axis axis) { return axis == Axis::X ? x() : y(); }
    constexpr const IntRange axis(Axis axis) const { return axis == Axis::X ? x() : y(); }
    constexpr int& left() { return x().low(); }
    constexpr int left() const { return x().low(); }
    constexpr int& right() { return x().high(); }
    constexpr int right() const { return x().high(); }
    constexpr int& bottom() { return y().low(); }
    constexpr int bottom() const { return y().low(); }
    constexpr int& top() { return y().high(); }
    constexpr int top() const { return y().high(); }
    constexpr int width() const { return x().extend(); }
    constexpr int height() const { return y().extend(); }
    constexpr IntVec bottomLeft() const { return { left(), bottom() }; }
    constexpr IntVec bottomRight() const { return { right(), bottom() }; }
    constexpr IntVec topLeft() const { return { left(), top() }; }
    constexpr IntVec topRight() const { return { right(), top() }; }
    constexpr IntVec dimensions() const { return { width(), height() }; }
    constexpr bool contains(const IntVec& v) const {
        return v.x() >= left() && v.x() <= right() && v.y() >= bottom() && v.y() <= top();
    }
    constexpr bool operator==(const IntRect&) const = default;
};

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
    Ref(const Ref<Size>& value)
        : Ref(Ref<Range>(value)) { }
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