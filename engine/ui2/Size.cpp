#include "Size.hpp"
#include "immintrin.h"
#include "util/target.hpp"

namespace ui {

#if defined(ARCHITECTURE_X86_64)

namespace {
    void asPixelsBase(const Size* in, float* out, int_t size, float ptScale, float payloadAdd) {
        for (int_t i = 0; i < size; i++)
            out[i] = in[i].asPixels(ptScale) + (in[i].payloadBit() ? payloadAdd : 0.0f);
    }
}

void Size::asPixelsImpl(const Size* in, float* out, int_t size, float ptScale, float payloadAddF) {
    __m256 shift = _mm256_set1_ps(expShift);
    __m256i payloadMask = _mm256_set1_epi32(payloadUintMask);
    __m256i unitMask = _mm256_set1_epi32(unitUintMask);
    __m256i flagsMask = _mm256_set1_epi32(tagMask);
    __m256 ptScaleAndShift = _mm256_mul_ps(shift, _mm256_set1_ps(ptScale));
    __m256 payloadAdd = _mm256_set1_ps(payloadAddF);

    int_t i = 0;
    int_t sizeFloor = (size_t)size & ~(size_t)7;
    for (; i < sizeFloor; i += 8) {
        __m256i packed = _mm256_loadu_si256((const __m256i*)(in + i));
        __m256 packedCleared = _mm256_castsi256_ps(_mm256_andnot_si256(flagsMask, packed));

        __m256i unitSel = _mm256_cmpeq_epi32(_mm256_and_si256(packed, unitMask), _mm256_setzero_si256());
        __m256 mul = _mm256_blendv_ps(ptScaleAndShift, shift, _mm256_castsi256_ps(unitSel));

        __m256i payloadSel = _mm256_cmpeq_epi32(_mm256_and_si256(packed, payloadMask), _mm256_setzero_si256());
        __m256 add = _mm256_blendv_ps(payloadAdd, _mm256_setzero_ps(), _mm256_castsi256_ps(payloadSel));

        __m256 value = _mm256_fmadd_ps(packedCleared, mul, add);
        _mm256_storeu_ps(out + i, value);
    }
    asPixelsBase(in + i, out + i, size - i, ptScale, payloadAddF);
}

#else

void Size::asPixelsImpl(const Size* in, float* out, int_t size, float ptScale, float payloadAdd) {
    asPixelsBase(in, out, size, ptScale, payloadAdd);
}

#endif

}