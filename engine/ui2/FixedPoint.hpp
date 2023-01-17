#pragma once

#include "util/types.hpp"
#include <array>
#include <bit>
#include <immintrin.h>

namespace ui {

struct FixedPoint {
    uint16_t value = 0;
    constexpr FixedPoint& operator+=(FixedPoint);
    constexpr FixedPoint& operator-=(FixedPoint);
    constexpr FixedPoint& operator*=(FixedPoint);
};
constexpr FixedPoint rawMultiply(FixedPoint l, FixedPoint r) {
    return { static_cast<uint16_t>(l.value * r.value) };
}
constexpr FixedPoint divideBy255(FixedPoint in) {
    return { static_cast<uint16_t>((static_cast<uint32_t>(in.value + 128) * (uint32_t)257) >> 16) };
}

// 16 x 16 bit unsigned integers
struct FixedPointLine {
    __m256i simd;
    FixedPointLine()
        : simd(_mm256_setzero_si256()) { }
    FixedPointLine(__m256i simd)
        : simd(simd) { }
    FixedPointLine(FixedPoint v)
        : simd(_mm256_set1_epi16(v.value)) { }

    FixedPointLine& operator+=(FixedPointLine);
    FixedPointLine& operator-=(FixedPointLine);
    FixedPointLine& operator*=(FixedPointLine);
};
inline FixedPointLine rawMultiply(FixedPointLine l, FixedPointLine r) {
    return _mm256_mullo_epi16(l.simd, r.simd);
}
inline FixedPointLine divideBy255(FixedPointLine in) {
    /*
     *  2^(2n) / (2^n - 1) - (2^n + 1)
     *  = (2^(2n) - (2^n + 1)(2^n - 1)) / (2^n - 1)
     *  = 1 / (2^n - 1)
     *
     *  d = (x + 2^(n-1)) * (2^n + 1) / 2^(2n) - (x *  / (2^n - 1) + 1/2)
     *
     *  d * 2^(2n) = (x + 2^(n-1)) * (2^n + 1) - (x * 2^(2n) / (2^n - 1) + 2^(2n - 1))
     *             = (x + 2^(n-1)) * (2^n + 1) - (x * (2^n + 1 + 1 / (2^n - 1)) + 2^(2n - 1))
     *             = x * (2^n + 1) + 2^(2n - 1) + 2^(n-1) - x * (2^n + 1) - x / (2^n - 1) - 2^(2n - 1)
     *             = 2^(n-1) - x / (2^n - 1)
     *
     *  d = (2^(n-1) - x / (2^n - 1)) / 2^(2n)
     *    = 2^(-n-1) - x / 2^(2n) / (2^n - 1)
     *
     *  x >= 0
     *  d <= 2^(-n-1)
     *
     *  x <= (2^n - 1) * 2^n
     *  d >= 1 / 2^(n+1) - 1 / 2^n = -2^(-n-1)
     */
    const __m256i multiply = _mm256_set1_epi16(257);
    const __m256i add = _mm256_set1_epi16(128);
    return _mm256_mulhi_epu16(_mm256_add_epi16(in.simd, add), multiply);
}

struct Color {
    // bgra8888
    std::array<uint8_t, 4> value = {};
    constexpr Color() = default;
    constexpr Color(uint32_t val)
        : value(std::bit_cast<std::array<uint8_t, 4>>(val)) { }
    constexpr FixedPoint alpha() const {
        return { value[3] };
    }
    constexpr FixedPoint red() const {
        return { value[2] };
    }
    constexpr FixedPoint green() const {
        return { value[1] };
    }
    constexpr FixedPoint blue() const {
        return { value[0] };
    }
    constexpr void setAlpha(FixedPoint v) {
        value[3] = std::min(v.value, (uint16_t)255);
    }
    constexpr void setRed(FixedPoint v) {
        value[2] = std::min(v.value, (uint16_t)255);
    }
    constexpr void setGreen(FixedPoint v) {
        value[1] = std::min(v.value, (uint16_t)255);
    }
    constexpr void setBlue(FixedPoint v) {
        value[0] = std::min(v.value, (uint16_t)255);
    }
    constexpr uint32_t uint() const {
        return std::bit_cast<uint32_t>(value);
    }
};

struct RGBALine;

struct RGBLine {
    FixedPointLine b, g, r;

    RGBLine() = default;
    RGBLine(FixedPointLine b, FixedPointLine g, FixedPointLine r)
        : b(b), g(g), r(r) { }
    RGBLine(Color c)
        : b(c.blue()), g(c.green()), r(c.red()) { }

    RGBALine withAlpha(FixedPointLine a) const;
};

struct RGBALine {
    FixedPointLine b, g, r, a;

    RGBALine() = default;
    RGBALine(FixedPointLine b, FixedPointLine g, FixedPointLine r, FixedPointLine a)
        : b(b), g(g), r(r), a(a) { }
    RGBALine(Color c)
        : b(c.blue()), g(c.green()), r(c.red()), a(c.alpha()) { }

    static RGBALine load(const uint32_t* in) {
        // { 0, 4, 8, 12, 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15 }
        const __m128i halfShuffle1 = _mm_set_epi8(15, 11, 7, 3, 14, 10, 6, 2, 13, 9, 5, 1, 12, 8, 4, 0);
        const __m256i shuffle1 = _mm256_set_m128i(halfShuffle1, halfShuffle1);
        // { 0, 4, 1, 5, 2, 6, 3, 7 }
        const __m256i shuffle2 = _mm256_set_epi32(7, 3, 6, 2, 5, 1, 4, 0);

        // input: { b, g, r, a }
        __m256i i1 = _mm256_loadu_si256((__m256i*)in);
        __m256i i2 = _mm256_loadu_si256((__m256i*)in + 1);
        i1 = _mm256_shuffle_epi8(i1, shuffle1);
        i2 = _mm256_shuffle_epi8(i2, shuffle1);
        i1 = _mm256_permutevar8x32_epi32(i1, shuffle2);
        i2 = _mm256_permutevar8x32_epi32(i2, shuffle2);
        __m256i br = _mm256_unpacklo_epi64(i1, i2);
        __m256i ga = _mm256_unpackhi_epi64(i1, i2);
        return {
            _mm256_cvtepu8_epi16(_mm256_extracti128_si256(br, 0)),
            _mm256_cvtepu8_epi16(_mm256_extracti128_si256(ga, 0)),
            _mm256_cvtepu8_epi16(_mm256_extracti128_si256(br, 1)),
            _mm256_cvtepu8_epi16(_mm256_extracti128_si256(ga, 1)),
        };
    }

    void store(uint32_t* out) const {
        __m256i bg = _mm256_or_si256(b.simd, _mm256_slli_epi16(g.simd, 8));
        __m256i ra = _mm256_or_si256(r.simd, _mm256_slli_epi16(a.simd, 8));
        __m256i i1 = _mm256_unpacklo_epi16(bg, ra);
        __m256i i2 = _mm256_unpackhi_epi16(bg, ra);
        _mm_storeu_si128((__m128i*)out, _mm256_extracti128_si256(i1, 0));
        _mm_storeu_si128((__m128i*)out + 1, _mm256_extracti128_si256(i2, 0));
        _mm_storeu_si128((__m128i*)out + 2, _mm256_extracti128_si256(i1, 1));
        _mm_storeu_si128((__m128i*)out + 3, _mm256_extracti128_si256(i2, 1));
    }

    RGBLine rgb() const {
        return { b, g, r };
    }
};

inline RGBALine RGBLine::withAlpha(FixedPointLine a) const {
    return { b, g, r, a };
}

struct U16Line {
    static U16Line indices() {
        return _mm256_set_epi16(15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0);
    }

    __m256i simd;
    U16Line()
        : simd(_mm256_setzero_si256()) { }
    U16Line(__m256i simd)
        : simd(simd) { }
    U16Line(uint16_t v)
        : simd(_mm256_set1_epi16(v)) { }

    U16Line& operator+=(U16Line);
    U16Line& operator-=(U16Line);
};

inline ui::U16Line min(ui::U16Line l, ui::U16Line r) {
    return _mm256_min_epu16(l.simd, r.simd);
}
inline ui::U16Line max(ui::U16Line l, ui::U16Line r) {
    return _mm256_max_epu16(l.simd, r.simd);
}
inline ui::U16Line choose(ui::U16Line c, ui::U16Line a, ui::U16Line b) {
    return _mm256_blendv_epi8(b.simd, a.simd, c.simd);
}
inline ui::FixedPointLine choose(ui::U16Line c, ui::FixedPointLine a, ui::FixedPointLine b) {
    return _mm256_blendv_epi8(b.simd, a.simd, c.simd);
}
inline ui::RGBLine choose(ui::U16Line c, ui::RGBLine a, ui::RGBLine b) {
    return { choose(c, a.b, b.b), choose(c, a.g, b.g), choose(c, a.r, b.r) };
}
inline ui::RGBALine choose(ui::U16Line c, ui::RGBALine a, ui::RGBALine b) {
    return { choose(c, a.b, b.b), choose(c, a.g, b.g), choose(c, a.r, b.r), choose(c, a.a, b.a) };
}

}

consteval ui::FixedPoint operator""_fp(long double v) {
    return { static_cast<uint16_t>(v * 255.0 + 0.5) };
}
constexpr ui::FixedPoint operator+(ui::FixedPoint l, ui::FixedPoint r) {
    return { static_cast<uint16_t>(l.value + r.value) };
}
constexpr ui::FixedPoint operator-(ui::FixedPoint l, ui::FixedPoint r) {
    return { static_cast<uint16_t>(l.value - r.value) };
}
constexpr ui::FixedPoint operator*(ui::FixedPoint l, ui::FixedPoint r) {
    return ui::divideBy255(ui::rawMultiply(l, r));
}

inline ui::FixedPointLine operator+(ui::FixedPointLine l, ui::FixedPointLine r) {
    return _mm256_add_epi16(l.simd, r.simd);
}
inline ui::FixedPointLine operator-(ui::FixedPointLine l, ui::FixedPointLine r) {
    return _mm256_sub_epi16(l.simd, r.simd);
}
inline ui::FixedPointLine operator*(ui::FixedPointLine l, ui::FixedPointLine r) {
    return ui::divideBy255(ui::rawMultiply(l, r));
}

inline ui::U16Line operator+(ui::U16Line l, ui::U16Line r) {
    return _mm256_add_epi16(l.simd, r.simd);
}
inline ui::U16Line operator-(ui::U16Line l, ui::U16Line r) {
    return _mm256_sub_epi16(l.simd, r.simd);
}
inline ui::U16Line operator^(ui::U16Line l, ui::U16Line r) {
    return _mm256_xor_si256(l.simd, r.simd);
}
inline ui::U16Line operator&(ui::U16Line l, ui::U16Line r) {
    return _mm256_and_si256(l.simd, r.simd);
}
inline ui::U16Line operator|(ui::U16Line l, ui::U16Line r) {
    return _mm256_or_si256(l.simd, r.simd);
}
inline ui::U16Line operator~(ui::U16Line in) {
    return _mm256_xor_si256(in.simd, _mm256_set1_epi8(0xff));
}
inline ui::U16Line operator==(ui::U16Line l, ui::U16Line r) {
    return _mm256_cmpeq_epi16(l.simd, r.simd);
}
inline ui::U16Line operator<=(ui::U16Line l, ui::U16Line r) {
    return l == ui::min(l, r);
}
inline ui::U16Line operator>=(ui::U16Line l, ui::U16Line r) {
    return l == ui::max(l, r);
}
inline ui::U16Line operator<(ui::U16Line l, ui::U16Line r) {
    return ~(l >= r);
}
inline ui::U16Line operator>(ui::U16Line l, ui::U16Line r) {
    return ~(l <= r);
}

constexpr ui::Color operator""_rgb(unsigned long long value) {
    return { (uint32_t)value | 0xff'00'00'00 };
}
constexpr ui::Color operator""_rgba(unsigned long long value) {
    return { (uint32_t)value };
}

namespace ui {

constexpr FixedPoint& FixedPoint::operator+=(FixedPoint r) {
    *this = *this + r;
    return *this;
}
constexpr FixedPoint& FixedPoint::operator-=(FixedPoint r) {
    *this = *this - r;
    return *this;
}
constexpr FixedPoint& FixedPoint::operator*=(FixedPoint r) {
    *this = *this * r;
    return *this;
}

inline FixedPointLine& FixedPointLine::operator+=(FixedPointLine r) {
    *this = *this + r;
    return *this;
}
inline FixedPointLine& FixedPointLine::operator-=(FixedPointLine r) {
    *this = *this - r;
    return *this;
}
inline FixedPointLine& FixedPointLine::operator*=(FixedPointLine r) {
    *this = *this * r;
    return *this;
}

inline U16Line& U16Line::operator+=(U16Line r) {
    *this = *this + r;
    return *this;
}
inline U16Line& U16Line::operator-=(U16Line r) {
    *this = *this + r;
    return *this;
}

inline RGBLine blend(RGBLine src, RGBLine dst, FixedPointLine srcFactor) {
    FixedPointLine dstFactor = 1.0_fp - srcFactor;
    return {
        divideBy255(rawMultiply(src.b, srcFactor) + rawMultiply(dst.b, dstFactor)),
        divideBy255(rawMultiply(src.g, srcFactor) + rawMultiply(dst.g, dstFactor)),
        divideBy255(rawMultiply(src.r, srcFactor) + rawMultiply(dst.r, dstFactor)),
    };
}

}