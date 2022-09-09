#include "Size.hpp"
#include "immintrin.h"
#include "util/target.hpp"

namespace ui {

#if defined(ARCHITECTURE_X86_64)

namespace {
    void asPixelsBase(const Size* in, float* out, int_t size, float ptScale) {
        for (int_t i = 0; i < size; i++)
            out[i] = in[i].asPixels(ptScale);
    }
}

void Size::asPixelsImpl(const Size* in, float* out, int_t size, float ptScale) {
    __m256 shift = _mm256_set1_ps(shiftF);
    __m256i mask = _mm256_set1_epi32(unitU32);
    __m256 ptScaleAndShift = ptScale * shift;

    int_t i = 0;
    int_t sizeFloor = (size_t)size & ~(size_t)7;
    for (; i < sizeFloor; i += 8) {
        __m256i packed = _mm256_loadu_si256((const __m256i*)(in + i));
        __m256 packedCleared = _mm256_castsi256_ps(_mm256_andnot_si256(mask, packed));
        __m256i sel = _mm256_cmpeq_epi32(_mm256_and_si256(packed, mask), _mm256_setzero_si256());
        __m256 mul = _mm256_blendv_ps(ptScaleAndShift, shift, _mm256_castsi256_ps(sel));
        __m256 value = packedCleared * mul;
        _mm256_storeu_ps(out + i, value);
    }
    asPixelsBase(in + i, out + i, size - i, ptScale);
}

#else

void Size::asPixelsImpl(const Size* in, float* out, int_t size, float ptScale) {
    asPixelsBase(in, out, size, ptScale);
}

#endif

}