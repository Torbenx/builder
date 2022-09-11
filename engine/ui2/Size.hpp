#pragma once

#include "util/log.hpp"
#include <bit>
#include <cmath>

namespace Tag {
struct Ui {
    static constexpr char tag[] = "ui";
    static constexpr bool verifyEnabled = true;
};
}

namespace ui {

enum class Axis {
    Default = 0,
    X = Default,
    Y,
};
enum class SizeUnit {
    Pixels,
    Points,
};
struct Size {
    static constexpr uint32_t unitU32 = (uint32_t)1 << 30;
    static constexpr uint32_t shiftU32 = (uint32_t)1 << 29;

    static constexpr float unitF = 0x1p128;
    static constexpr float shiftF = 0x1p64;

    static constexpr float normalizedMin = 0x1p-62;

    /*
     * storage:
     * bit     | 31 | 30 | 29 - 23 | 22 - 0 |
     * content |  s |  u |     exp |   frac |
     * value = (-1)^s * (1.frac) * 2^(exp - 63)
     * unit  = u ? Points : Pixels
     *
     * interpretation as float (while normalized):
     * storage = value * 2^(-64) * (u ? 2^128 : 1)
     *
     * interpretation as uint32 (while normaized):
     * storage = value - 2^29 + (u ? 2^30 : 0)
     */
    float storage = 0.0f;

    constexpr Size() { }
    constexpr Size(float value, SizeUnit unit) {
        if (std::is_constant_evaluated()) {
            if (value == 0.0f) {
                storage = 0.0f;
            } else {
                float absValue = value > 0.0f ? value : -value;
                VERIFY(Ui, absValue >= normalizedMin); // value must be normalized
                storage = value / shiftF;
                if (unit == SizeUnit::Points)
                    storage *= unitF;
            }
        } else {
            storage = std::bit_cast<float>(std::bit_cast<uint32_t>(value / shiftF) | (unit == SizeUnit::Points ? unitU32 : (uint32_t)0));
        }
    }
    static constexpr Size fromStorage(float storage) { return Size { storage }; }

    constexpr float value() const {
        if (std::is_constant_evaluated()) {
            float val = storage * shiftF;
            if (unitBit())
                val /= unitF;
            return val;
        } else {
            return std::bit_cast<float>(std::bit_cast<uint32_t>(storage) & ~unitU32) * shiftF;
        }
    }
    constexpr bool unitBit() const {
        if (std::is_constant_evaluated()) {
            if (storage == 0.0f)
                return false;
            float absStorage = storage > 0.0f ? storage : -storage;
            /*            | value         | storage (u=0)  | storage (u=1)
             * min (incl) | 1.0 * 2^(-62) | 1.0 * 2^(-126) | 1.0 * 2^(+  2)
             * max (excl) | 2.0 * 2^(+64) | 2.0 * 2^(+  0) | 2.0 * 2^(+128)
             */
            // no values should lie between 2 and 4
            return absStorage > 3.0f;
        } else {
            return std::bit_cast<uint32_t>(storage) & unitU32;
        }
    }
    constexpr float asPixels(float ptScale) const {
        if (std::is_constant_evaluated()) {
            float val = value();
            if (unitBit())
                val *= ptScale;
            return val;
        } else {
            return value() * (unitBit() ? ptScale : 1.0f);
        }
    }
    static constexpr void asPixels(const Size* in, float* out, int_t size, float ptScale) {
        if (std::is_constant_evaluated()) {
            for (int_t i = 0; i < size; i++)
                out[i] = in[i].asPixels(ptScale);
        } else {
            asPixelsImpl(in, out, size, ptScale);
        }
    }

    constexpr bool inPt() const { return unitBit(); }
    constexpr bool inPx() const { return !inPt(); }
    constexpr SizeUnit unit() const { return unitBit() ? SizeUnit::Points : SizeUnit::Pixels; }

private:
    explicit constexpr Size(float storage)
        : storage(storage) { }
    static void asPixelsImpl(const Size* in, float* out, int_t size, float ptScale);
};
inline namespace code {
    inline namespace operators {
        constexpr Size operator-(const Size& in) { return Size::fromStorage(-in.storage); }
        constexpr Size operator*(float l, const Size& r) { return Size::fromStorage(l * r.storage); }
        constexpr Size operator*(const Size& l, float r) { return Size::fromStorage(l.storage * r); }
        constexpr Size operator/(const Size& l, float r) { return Size::fromStorage(l.storage / r); }
    }

    inline namespace literals {
        constexpr Size operator"" _px(unsigned long long value) { return { (float)value, SizeUnit::Pixels }; }
        constexpr Size operator"" _pt(unsigned long long value) { return { (float)value, SizeUnit::Points }; }
        constexpr Size operator"" _px(long double value) { return { (float)value, SizeUnit::Pixels }; }
        constexpr Size operator"" _pt(long double value) { return { (float)value, SizeUnit::Points }; }
    }
}

struct Range {
    Size m_low, m_high;
    constexpr Range() { }
    constexpr Range(const Size& value)
        : Range(value, value) { }
    constexpr Range(const Size& low, const Size& high)
        : m_low(low), m_high(high) { }
    constexpr Size& low() { return m_low; }
    constexpr const Size& low() const { return m_low; }
    constexpr Size& high() { return m_high; }
    constexpr const Size& high() const { return m_high; }
};
constexpr Range operator-(const Range& in) {
    return { -in.low(), -in.high() };
}

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
    constexpr const Range& x() const { return m_x; }
    constexpr Range& y() { return m_y; }
    constexpr const Range& y() const { return m_y; }
    constexpr Range& axis(Axis axis) { return axis == Axis::X ? x() : y(); }
    constexpr const Range& axis(Axis axis) const { return axis == Axis::X ? x() : y(); }
    constexpr Size& left() { return x().low(); }
    constexpr const Size& left() const { return x().low(); }
    constexpr Size& right() { return x().high(); }
    constexpr const Size& right() const { return x().high(); }
    constexpr Size& bottom() { return y().low(); }
    constexpr const Size& bottom() const { return y().low(); }
    constexpr Size& top() { return y().high(); }
    constexpr const Size& top() const { return y().high(); }
};
constexpr Rect operator-(const Rect& in) {
    return { -in.x(), -in.y() };
}

}