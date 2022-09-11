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

template<int N>
struct TagFloat {
    static_assert(N >= 0);
    static_assert(N <= 6);

    // storage:
    // bit     | 31 | 30 - (30-N+1) | (30-N) - 23 | 22 - 0 |
    // content |  s |           tag |         exp |   frac |
    float storage = 0.0f;
    // value = (-1)^s * (1.frac) * 2^(exp - 2^(expBits - 1) + 1)

    static constexpr int tagBits = N;
    static constexpr int expBits = 8 - tagBits;
    static constexpr int tagPos = 31 - tagBits;
    static constexpr uint32_t tagMask = (((uint32_t)1 << tagBits) - 1) << tagPos;
    static constexpr uint8_t tagMax = (uint8_t)1 << tagBits;

    // maps value exponent range [2^(expBits - 1), -2^(expBits - 1) + 1]
    // to storage exponent range [2^expBits - 128, -127]
    // static constexpr int expShiftExp = (1 << (expBits - 1)) - ((1 << (expBits)) - 128);
    static constexpr int expShiftExp = 128 - (1 << (expBits - 1));
    static constexpr float expShift = 0x1p64 * ((uint64_t)1 << (expShiftExp - 64));

    // interpretation as float (while normalized):
    // storage = value / expShift * 2^(tag * 2^expBits)

    static constexpr int normaizedMinExp = (1 << (expBits - 1)) - 2;
    static constexpr float normalizedMin = 1.0f / ((uint64_t)1 << normaizedMinExp); // this will break with N=0

    constexpr TagFloat() { }
    constexpr TagFloat(float value, uint8_t tag) {
        VERIFY(Ui, tag < tagMax);
        VERIFY(Ui, value == 0.0f || std::abs(value) >= normalizedMin); // we can store subnormals but cant multiply them
        storage = std::bit_cast<float>(std::bit_cast<uint32_t>(value / expShift) | ((uint32_t)tag << tagPos));
    }
    static constexpr TagFloat fromStorage(float storage) { return TagFloat { storage }; }
    static constexpr TagFloat fromUint(uint32_t uint) { return fromStorage(std::bit_cast<float>(uint)); }

    constexpr uint32_t uint() const { return std::bit_cast<uint32_t>(storage); }
    constexpr float value() const { return std::bit_cast<float>(uint() & ~tagMask) * expShift; }
    constexpr uint8_t tag() const { return (uint8_t)((uint() & tagMask) >> tagPos); }
    constexpr TagFloat orTag(uint8_t tag) const { return fromUint(uint() | ((tag << tagPos) & tagMask)); }
    constexpr TagFloat andTag(uint8_t tag) const { return fromUint(uint() & ((tag << tagPos) | ~tagMask)); }

private:
    TagFloat(float storage)
        : storage(storage) { }
};

inline namespace code {
    template<int N>
    constexpr TagFloat<N> operator-(const TagFloat<N>& f) { return TagFloat<N>::fromStorage(-f.storage); }
    template<int N>
    constexpr TagFloat<N> operator*(float l, const TagFloat<N>& r) { return TagFloat<N>::fromStorage(l * r.storage); }
    template<int N>
    constexpr TagFloat<N> operator*(const TagFloat<N>& l, float r) { return TagFloat<N>::fromStorage(l.storage * r); }
    template<int N>
    constexpr TagFloat<N> operator/(const TagFloat<N>& l, float r) { return TagFloat<N>::fromStorage(l.storage / r); }
}

enum class SizeUnit {
    Pixels,
    Points,
};
struct Size : TagFloat<2> {
    static constexpr uint8_t payloadTagMask = 0b10;
    static constexpr uint8_t unitTagMask = 0b01;
    static constexpr uint32_t payloadUintMask = (uint32_t)payloadTagMask << tagPos;
    static constexpr uint32_t unitUintMask = (uint32_t)unitTagMask << tagPos;
    constexpr Size() { }
    constexpr Size(float value, SizeUnit unit, bool payload = false)
        : TagFloat<2>(value, (unit == SizeUnit::Points ? unitTagMask : 0) | (payload ? payloadTagMask : 0)) { }
    constexpr Size(TagFloat<2> storage)
        : TagFloat<2>(storage) { }
    constexpr bool unitBit() const { return tag() & unitTagMask; }
    constexpr bool payloadBit() const { return tag() & payloadTagMask; }
    constexpr bool inPt() const { return unitBit(); }
    constexpr bool inPx() const { return !inPt(); }
    constexpr SizeUnit unit() const { return unitBit() ? SizeUnit::Points : SizeUnit::Pixels; }
    constexpr Size withPayload() const { return orTag(payloadTagMask); }
    constexpr Size withoutPayload() const { return andTag(~payloadTagMask); }
    constexpr float asPixels(float ptScale) const { return value() * (inPt() ? ptScale : 0.0f); }

    static constexpr void asPixels(const Size* in, float* out, int_t size, float ptScale, float payloadAddF) {
        if (std::is_constant_evaluated()) {
            for (int_t i = 0; i < size; i++)
                out[i] = in[i].asPixels(ptScale) + (in[i].payloadBit() ? payloadAddF : 0.0f);
        } else {
            asPixelsImpl(in, out, size, ptScale, payloadAddF);
        }
    }

private:
    static void asPixelsImpl(const Size* in, float* out, int_t size, float ptScale, float payloadAddF);
};

inline namespace code {
    inline namespace operators {
        constexpr Size operator-(const Size& in) { return -(const TagFloat<2>&)in; }
        constexpr Size operator*(float l, const Size& r) { return l * (const TagFloat<2>&)r; }
        constexpr Size operator*(const Size& l, float r) { return (const TagFloat<2>&)l * r; }
        constexpr Size operator/(const Size& l, float r) { return (const TagFloat<2>&)l / r; }
    }

    inline namespace literals {
        constexpr Size operator"" _px(unsigned long long value) { return { (float)value, SizeUnit::Pixels }; }
        constexpr Size operator"" _pt(unsigned long long value) { return { (float)value, SizeUnit::Points }; }
        constexpr Size operator"" _px(long double value) { return { (float)value, SizeUnit::Pixels }; }
        constexpr Size operator"" _pt(long double value) { return { (float)value, SizeUnit::Points }; }
    }
}

}