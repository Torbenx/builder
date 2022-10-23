#pragma once

#include "containers/Owned.hpp"
#include "ui2/IntTypes.hpp"

namespace sw {

using IntVec = ui::IntVec;

struct TexturePosition {
    uint32_t* data = nullptr;
    int_t stride = 0;
    constexpr uint32_t* operator[](int_t j) const { return data + j * stride; }
    constexpr TexturePosition shift(const IntVec& shift) const {
        return { data + shift.y() * stride + shift.x(), stride };
    }
};

struct TextureView {
    TexturePosition position;
    IntVec dimensions;

    void deallocate() {
        std::allocator<uint32_t>().deallocate(position.data, position.stride * dimensions.y());
        *this = {};
    }
    TexturePosition pos() const { return position; }
};

inline TextureView allocateTexture(IntVec dimensions) {
    int_t stride = stx::alignmentCeil(16, dimensions.x());
    uint32_t* data = std::allocator<uint32_t>().allocate(stride * dimensions.y());
    return { { data, stride }, dimensions };
}

}

constexpr sw::TexturePosition operator+(const sw::TexturePosition& view, const sw::IntVec& shift) {
    return view.shift(shift);
}
constexpr sw::TexturePosition operator+(const sw::IntVec& shift, const sw::TexturePosition& view) {
    return view.shift(shift);
}
constexpr sw::TexturePosition operator-(const sw::TexturePosition& view, const sw::IntVec& shift) {
    return view.shift(-shift);
}