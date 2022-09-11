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

enum class SizeUnit {
    Pixels,
    Points,
};
struct Size {

    /*
     * storage:
     * bit     | 31 | 30 | 29 | 28 - 23 | 22 - 0 |
     * content |  s |  o |  u |     exp |   frac |
     * value = (-1)^s * (1.frac) * 2^(exp - 31)
     * payload = p
     * unit  = u ? Points : Pixels
     *
     * interpretation as float (while normalized):
     * storage = value * 2^(-96) * (u ? 2^64 : 1) * (p ? 2^128 : 1)
     */
    float storage = 0.0f;

    static constexpr uint32_t payloadU32 = (uint32_t)1 << 30;
    static constexpr uint32_t unitU32 = (uint32_t)1 << 29;
    static constexpr uint32_t flagsMask = unitU32 | payloadU32;

    static constexpr float payloadF = 0x1p128;
    static constexpr float unitF = 0x1p64;
    static constexpr float shiftF = 0x1p96;

    static constexpr float normalizedMin = 0x1p-30;

    //            | value         | storage (p=0,u=0) | storage (p=0,u=1) | storage (p=1,u=0) | storage (p=1,u=1)
    // min (incl) | 1.0 * 2^(-30) |    1.0 * 2^(-126) |    1.0 * 2^(- 62) |    1.0 * 2^(+  2) |    1.0 * 2^(- 66)
    // max (excl) | 2.0 * 2^(+32) |    2.0 * 2^(- 64) |    2.0 * 2^(+  0) |    2.0 * 2^(+ 64) |    2.0 * 2^(+128)
    static constexpr float payloadBitBoundary = (0x2p0 + 0x1p2) / 2;
    static constexpr float unitBitBoundary = (0x2p-64 + 0x1p-62) / 2; // assuming p=0

    constexpr Size() { }
    constexpr Size(float value, SizeUnit unit, bool payload = false) {
        if (std::is_constant_evaluated()) {
            if (value == 0.0f) {
                VERIFY(Ui, !payload); // cannot represent p=1 and value=0.0 at compile time
                storage = 0.0f;
            } else {
                float absValue = value > 0.0f ? value : -value;
                VERIFY(Ui, absValue >= normalizedMin); // value must be normalized
                storage = value / shiftF;
                if (payload)
                    storage *= payloadF;
                if (unit == SizeUnit::Points)
                    storage *= unitF;
            }
        } else {
            storage = std::bit_cast<float>(
                std::bit_cast<uint32_t>(value / shiftF)
                | (payload ? payloadU32 : (uint32_t)0)
                | (unit == SizeUnit::Points ? unitU32 : (uint32_t)0));
        }
    }
    static constexpr Size fromStorage(float storage) { return Size { storage }; }
    static Size fromUint(uint32_t uint) { return fromStorage(std::bit_cast<float>(uint)); }

    constexpr float value() const {
        if (std::is_constant_evaluated()) {
            float absStorage = storage > 0.0f ? storage : -storage;
            float value = storage;
            if (absStorage > payloadBitBoundary) {
                absStorage /= payloadF;
                value /= payloadF;
            }
            if (absStorage > unitBitBoundary)
                value /= unitF;
            return value * shiftF;
        } else {
            return std::bit_cast<float>(uint() & ~flagsMask) * shiftF;
        }
    }
    constexpr bool unitBit() const {
        if (std::is_constant_evaluated()) {
            if (storage == 0.0f)
                return false;
            float absStorage = storage > 0.0f ? storage : -storage;
            if (absStorage > payloadBitBoundary)
                absStorage /= payloadF;
            return absStorage > unitBitBoundary;
        } else {
            return uint() & unitU32;
        }
    }
    constexpr bool payloadBit() const {
        if (std::is_constant_evaluated()) {
            if (storage == 0.0f)
                return false;
            float absStorage = storage > 0.0f ? storage : -storage;
            return absStorage > payloadBitBoundary;
        } else {
            return uint() & payloadU32;
        }
    }

    constexpr float asPixels(float ptScale, float payloadAdd = 0.0f) const {
        return value() * (unitBit() ? ptScale : 1.0f) + (payloadBit() ? payloadAdd : 0.0f);
    }
    constexpr Size withPayload() const {
        if (std::is_constant_evaluated())
            return { value(), unit(), true };
        else
            return Size::fromUint(uint() | payloadU32);
    }
    constexpr Size withoutPayload() const {
        if (std::is_constant_evaluated())
            return { value(), unit(), false };
        else
            return Size::fromUint(uint() & ~payloadU32);
    }
    static constexpr void asPixels(const Size* in, float* out, int_t size, float ptScale, float payloadAdd = 0.0f) {
        if (std::is_constant_evaluated()) {
            for (int_t i = 0; i < size; i++)
                out[i] = in[i].asPixels(ptScale, payloadAdd);
        } else {
            asPixelsImpl(in, out, size, ptScale, payloadAdd);
        }
    }

    constexpr bool inPt() const { return unitBit(); }
    constexpr bool inPx() const { return !inPt(); }
    constexpr SizeUnit unit() const { return unitBit() ? SizeUnit::Points : SizeUnit::Pixels; }

    uint32_t uint() const { return std::bit_cast<uint32_t>(storage); }

private:
    static void asPixelsImpl(const Size* in, float* out, int_t size, float ptScale, float payloadAdd);
    explicit constexpr Size(float storage)
        : storage(storage) { }
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

}