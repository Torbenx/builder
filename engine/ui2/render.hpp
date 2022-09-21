#pragma once

#include "Rect.hpp"

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

struct RendererBase {
    uint32_t id = 0;
    ~RendererBase();
    constexpr explicit operator bool() const { return id != 0; }
};
template<typename T>
concept Renderer = std::derived_from<T, RendererBase> && requires(T& renderer, TextureView view, IntVec dimensions) { renderer.updateTexture(view, dimensions); };

struct RendererHandle {
    RendererBase* base = nullptr;
    void (*updatePtr)(RendererBase*, TextureView, IntVec) = nullptr;
    RendererBase* operator->() const { return base; }
    void update(TextureView view, IntVec dimensions) const { updatePtr(base, view, dimensions); }
};

void draw(RendererHandle, RectVarHandle);
void cancelDraw(RendererBase&);
template<Renderer T>
void draw(T& renderer, RectVarHandle rect) {
    RendererHandle handle;
    handle.base = (RendererBase*)&renderer;
    handle.updatePtr = [](RendererBase* base, TextureView view, IntVec dimensions) {
        static_cast<T*>(base)->updateTexture(view, dimensions);
    };
    draw(handle, rect);
}
inline RendererBase::~RendererBase() {
    if (*this)
        cancelDraw(*this);
}

}