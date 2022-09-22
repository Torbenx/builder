#pragma once

#include "Rect.hpp"
#include "event/Script.hpp"

namespace ui {

struct TextureView {
    struct LineView {
        uint32_t* data = nullptr;
        constexpr uint32_t& operator[](int_t i) const { return data[i]; }
    };
    uint32_t* data = nullptr;
    int_t stride = 0;
    constexpr LineView operator[](int_t j) const { return { data + j * stride }; }
    constexpr TextureView shift(const IntVec& shift) const {
        return { data + shift.y() * stride + shift.x(), stride };
    }
};

}

constexpr ui::TextureView operator+(const ui::TextureView& view, const ui::IntVec& shift) {
    return view.shift(shift);
}
constexpr ui::TextureView operator+(const ui::IntVec& shift, const ui::TextureView& view) {
    return view.shift(shift);
}
constexpr ui::TextureView operator-(const ui::TextureView& view, const ui::IntVec& shift) {
    return view.shift(-shift);
}

namespace ui {

struct TextureHandle {
    uint32_t* data = nullptr;
    IntVec dimensions;
    constexpr TextureView view() const { return { data, dimensions.x() }; }
    constexpr explicit operator bool() const { return data != nullptr; }
};
struct AtlasAllocator {
    virtual TextureHandle allocateAtlas(IntVec prevSize) = 0;
    virtual void deallocateAtlas(TextureHandle texture) = 0;
};

struct TextureState {
    TextureView texture;
    IntVec dimensions;
};
struct RendererBase {
    uint32_t id = 0;
    TextureState textureState() const;
    ~RendererBase();
    constexpr explicit operator bool() const { return id != 0; }
};
template<typename T>
concept Renderer = std::derived_from<T, RendererBase> && requires(T& renderer) { { renderer.textureUpdateSignal() } -> std::same_as<evt::Signal>; };

struct RendererHandle {
    RendererBase* base = nullptr;
    evt::Signal signal;
    RendererBase* operator->() const { return base; }
};

void draw(RendererHandle, RectVarHandle);
void cancelDraw(RendererBase&);
template<Renderer T>
void draw(T& renderer, RectVarHandle rect) {
    RendererHandle handle;
    handle.base = (RendererBase*)&renderer;
    handle.signal = renderer.textureUpdateSignal();
    draw(handle, rect);
}
inline RendererBase::~RendererBase() {
    if (*this)
        cancelDraw(*this);
}

}