#pragma once

#include "Semaphore.hpp"
#include "containers/Array.hpp"
#include "ui2/IntTypes.hpp"
#include "util/types.hpp"

namespace gfx {

enum class TexelLayout {
    Unknown,
    D16,
    RGBA8,
    BGRA8,
};

}

namespace sw {

using iVec2 = ui::IntVec;

using TexelLayout = gfx::TexelLayout;

template<TexelLayout layout>
struct texel_helper;
#define DEFINE_TEXEL(layout, ...)              \
    template<>                                 \
    struct texel_helper<TexelLayout::layout> { \
        using type = __VA_ARGS__;              \
    };
DEFINE_TEXEL(D16, uint16_t);
DEFINE_TEXEL(RGBA8, stx::Array<uint8_t, 4>);
DEFINE_TEXEL(BGRA8, uint32_t);

template<TexelLayout layout>
using texel_t = typename texel_helper<layout>::type;

template<TexelLayout layout = TexelLayout::Unknown>
struct ImagePtr;

template<>
struct ImagePtr<TexelLayout::Unknown> {
    std::byte* m_data = nullptr;
    uint32_t texelStrideInBytes = 0;
    uint32_t rowStrideInBytes = 0;

    ImagePtr& operator+=(const iVec2& vec) {
        m_data += vec.x() * texelStrideInBytes + vec.y() * rowStrideInBytes;
        return *this;
    }
    ImagePtr& operator-=(const iVec2& vec) {
        m_data -= vec.x() * texelStrideInBytes + vec.y() * rowStrideInBytes;
        return *this;
    }
    explicit operator bool() const { return (bool)m_data; }
    void* data() { return m_data; }
};

template<TexelLayout layout>
struct ImagePtr {
    using tex_t = texel_t<layout>;
    tex_t* m_data = nullptr;
    int_t rowStrideInTexels = 0;

    ImagePtr() = default;
    ImagePtr(tex_t* data, int_t stride)
        : m_data(data), rowStrideInTexels(stride) { }
    explicit ImagePtr(ImagePtr<> ptr)
        : m_data((tex_t*)ptr.data())
        , rowStrideInTexels(ptr.rowStrideInBytes / sizeof(tex_t)) {
        EXPECT(Software, (size_t)ptr.texelStrideInBytes, sizeof(tex_t));
    }

    ImagePtr& operator+=(const iVec2& vec) {
        m_data += vec.x() + vec.y() * rowStrideInTexels;
        return *this;
    }
    ImagePtr& operator-=(const iVec2& vec) {
        m_data -= vec.x() + vec.y() * rowStrideInTexels;
        return *this;
    }
    tex_t* row(int_t y) {
        return data() + y * rowStrideInTexels;
    }
    tex_t& operator[](const iVec2& vec) {
        return data()[vec.x() + vec.y() * rowStrideInTexels];
    }
    operator ImagePtr<>() {
        return {
            .m_data = (std::byte*)data(),
            .texelStrideInBytes = (uint32_t)sizeof(tex_t),
            .rowStrideInBytes = (uint32_t)(rowStrideInTexels * sizeof(tex_t)),
        };
    }
    tex_t* data() { return m_data; }
    int_t stride() const { return rowStrideInTexels; }
    explicit operator bool() const { return (bool)m_data; }
};

template<TexelLayout layout>
inline ImagePtr<layout> operator+(ImagePtr<layout> img, const iVec2& vec) {
    img += vec;
    return img;
}
template<TexelLayout layout>
inline ImagePtr<layout> operator+(const iVec2& vec, ImagePtr<layout> img) {
    img += vec;
    return img;
}
template<TexelLayout layout>
inline ImagePtr<layout> operator-(ImagePtr<layout> img, const iVec2& vec) {
    img -= vec;
    return img;
}

template<TexelLayout layout = TexelLayout::Unknown>
struct ImageView;

ImageView<> allocateImage(int_t texelSize, iVec2 dimensions);
void deallocateImage(ImageView<> image);

template<TexelLayout layout>
struct ImageView : ImagePtr<layout> {
    iVec2 m_dimensions;

    ImageView() = default;
    ImageView(ImagePtr<layout> ptr, iVec2 dimensions)
        : ImagePtr<layout>(ptr), m_dimensions(dimensions) { }
    ImageView(ImagePtr<> ptr, iVec2 dimensions) requires(layout != TexelLayout::Unknown)
        : ImagePtr<layout>(ptr), m_dimensions(dimensions) { }
    explicit ImageView(const ImageView<>& view) requires(layout != TexelLayout::Unknown)
        : ImageView((const ImagePtr<>&)view, view.dimensions()) { }
    ImageView(const ImageView&) = default;
    ImageView& operator=(const ImageView&) = default;

    iVec2 dimensions() const { return m_dimensions; }
    int_t sizeInTexels() const requires(layout != TexelLayout::Unknown) {
        return this->stride() * dimensions().y();
    }
    int_t sizeInBytes() const requires(layout != TexelLayout::Unknown) {
        return sizeof(texel_t<layout>) * sizeInTexels();
    }
    int_t sizeInBytes() const requires(layout == TexelLayout::Unknown) {
        return this->rowStrideInBytes * dimensions().y();
    }

    ImageView& operator+=(const iVec2& vec) {
        *(ImagePtr<layout>*)this += vec;
        return *this;
    }
    ImageView& operator-=(const iVec2& vec) {
        *(ImagePtr<layout>*)this -= vec;
        return *this;
    }
    operator ImageView<>() requires(layout != TexelLayout::Unknown) {
        return { (ImagePtr<>)*this, dimensions() };
    }

    void deallocate() requires(layout != TexelLayout::Unknown) {
        deallocateImage(*this);
        *this = {};
    }
    static ImageView allocate(iVec2 dimensions) requires(layout != TexelLayout::Unknown) {
        return (ImageView)allocateImage(sizeof(texel_t<layout>), dimensions);
    }
};
inline ImageView<> allocateImage(int_t texelSize, iVec2 dimensions) {
    int_t stride = stx::alignmentCeil(16, dimensions.x());
    std::byte* data = std::allocator<std::byte>().allocate(stride * dimensions.y() * texelSize);
    return {
        ImagePtr<> {
            .m_data = data,
            .texelStrideInBytes = (uint32_t)texelSize,
            .rowStrideInBytes = (uint32_t)(stride * texelSize),
        },
        dimensions,
    };
}
inline void deallocateImage(ImageView<> image) {
    std::allocator<std::byte>().deallocate((std::byte*)image.data(), image.sizeInBytes());
}

template<TexelLayout layout>
inline ImageView<layout> operator+(ImageView<layout> img, const iVec2& vec) {
    img += vec;
    return img;
}
template<TexelLayout layout>
inline ImageView<layout> operator+(const iVec2& vec, ImageView<layout> img) {
    img += vec;
    return img;
}
template<TexelLayout layout>
inline ImageView<layout> operator-(ImageView<layout> img, const iVec2& vec) {
    img -= vec;
    return img;
}

}