#pragma once

#include "BigStyle.hpp"
#include "Rect.hpp"
#include "containers/IdAllocator.hpp"
#include "math/sparse/Vector.hpp"
#include "util/types.hpp"
#include <memory>
#include <vector>


namespace ui {

struct Variable {
    uint32_t id = 0;
};
struct RangeVar {
    uint32_t baseId = 0;
    constexpr const Variable low() const { return { baseId }; }
    constexpr const Variable high() const { return { baseId + 1 }; }
};
struct RectVar {
    uint32_t baseId = 0;
    constexpr const RangeVar x() const { return { baseId }; }
    constexpr const RangeVar y() const { return { baseId + 2 }; }
    constexpr const RangeVar axis(Axis axis) const { return axis == Axis::X ? x() : y(); }
    constexpr const Variable left() const { return x().low(); }
    constexpr const Variable right() const { return x().high(); }
    constexpr const Variable bottom() const { return y().low(); }
    constexpr const Variable top() const { return y().high(); }
};
struct Equation {
    uint32_t id = 0;
};
struct RangeEqn {
    Equation m_low, m_high;
    constexpr Equation& low() { return m_low; }
    constexpr const Equation low() const { return m_low; }
    constexpr Equation& high() { return m_high; }
    constexpr const Equation high() const { return m_high; }
};
struct RectEqn {
    RangeEqn m_x, m_y;
    constexpr RangeEqn& x() { return m_x; }
    constexpr const RangeEqn x() const { return m_x; }
    constexpr RangeEqn& y() { return m_y; }
    constexpr const RangeEqn y() const { return m_y; }
    constexpr RangeEqn& axis(Axis axis) { return axis == Axis::X ? x() : y(); }
    constexpr const RangeEqn axis(Axis axis) const { return axis == Axis::X ? x() : y(); }
    constexpr Equation& left() { return x().low(); }
    constexpr const Equation left() const { return x().low(); }
    constexpr Equation& right() { return x().high(); }
    constexpr const Equation right() const { return x().high(); }
    constexpr Equation& bottom() { return y().low(); }
    constexpr const Equation bottom() const { return y().low(); }
    constexpr Equation& top() { return y().high(); }
    constexpr const Equation top() const { return y().high(); }
};

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
constexpr TextureView operator+(const TextureView& view, const IntVec& shift) {
    return view.shift(shift);
}
constexpr TextureView operator+(const IntVec& shift, const TextureView& view) {
    return view.shift(shift);
}
constexpr TextureView operator-(const TextureView& view, const IntVec& shift) {
    return view.shift(-shift);
}
struct Texture {
    uint32_t* data = nullptr;
    IntVec dimensions;
    constexpr Texture() = default;
    constexpr Texture(IntVec dimensions)
        : dimensions(dimensions) {
        std::allocator<uint32_t> alloc;
        data = alloc.allocate(dimensions.x() * dimensions.y());
    }
    constexpr Texture(Texture&& other)
        : data(other.data), dimensions(other.dimensions) {
        other.data = nullptr;
        other.dimensions = {};
    }
    constexpr Texture& operator=(Texture&& other) {
        maybeDeallocate();
        data = other.data;
        dimensions = other.dimensions;
        other.data = nullptr;
        other.dimensions = {};
        return *this;
    }
    constexpr ~Texture() {
        maybeDeallocate();
    }
    constexpr operator TextureView() const {
        return { data, dimensions.x() };
    }
    constexpr TextureView view() const { return *this; }
private:
    constexpr void maybeDeallocate() {
        if (data) {
            std::allocator<uint32_t> alloc;
            alloc.deallocate(data, dimensions.x() * dimensions.y());
        }
    }
};

struct RendererBase {
    uint32_t id = 0;
};
template<typename T>
concept Renderer = std::derived_from<T, RendererBase> && requires(T& render, TextureView view, IntVec dimensions) { render.updateTexture(view, dimensions); };

class Layout {
public:
    Layout();
    Layout(const Layout&) = delete;
    ~Layout();

    Variable newVar();
    RangeVar newRange();
    RectVar newRect();
    Equation align(Variable leftBottom, Variable rightTop, Size inhom = {});
    RangeEqn align(RangeVar inner, RangeVar outer, Range inhom = {});
    RectEqn align(RectVar inner, RectVar outer, Rect inhom = {});
    void updateInhom(Equation, Size inhom = {});
    void updateInhom(RangeEqn, Range inhom = {});
    void updateInhom(RectEqn, Rect inhom = {});
    void removeEqn(Equation);
    void removeEqn(RangeEqn);
    void removeEqn(RectEqn);
    void deleteVariable(Variable);
    void deleteVariable(RangeVar);
    void deleteVariable(RectVar);

    void resize(IntVec windowSize, float ptScale);
    void update();
    void doDraw();
    IntRect intRect(RectVar var) const { return intRect(var, varValues.data()); }

    RectVar screen() const { return screenBox; }
    void debug(RectVar box) const {
        log("Box: {}, {} - {}, {}", varValues[box.left().id], varValues[box.bottom().id], varValues[box.right().id], varValues[box.top().id]);
    }

    template<Renderer T>
    void draw(T& renderer, RectVar rect) {
        RendererHandle handle;
        handle.base = (RendererBase*)&renderer;
        handle.updatePtr = [](RendererBase* base, TextureView view, IntVec dimensions) {
            static_cast<T*>(base)->updateTexture(view, dimensions);
        };
        draw(handle, rect);
    }
    void cancelDraw(RendererBase&);

private:
    struct RendererHandle {
        RendererBase* base = nullptr;
        void (*updatePtr)(RendererBase*, TextureView, IntVec) = nullptr;
        RendererBase* operator->() const { return base; }
        void update(TextureView view, IntVec dimensions) const { updatePtr(base, view, dimensions); }
    };
    struct Render {
        RendererHandle handle;
        RectVar rect;
        IntVec allocPosition = {};
    };

    IdAllocator<uint32_t> eqnIdAlloc;
    uint32_t nextVarId = 4;
    std::vector<Size> inhomTyped;
    std::vector<float> inhomValues;
    std::vector<float> varValues;
    sparse::Matrix matrix;
    RectVar screenBox;
    RectEqn screenEqn;
    std::vector<Render> renders { 1 };

    uint32_t allocateVariableIds(int_t count);
    Equation makeEquation(std::initializer_list<sparse::Entry>, Size inhom = {});
    void draw(RendererHandle, RectVar);
    static IntRect intRect(RectVar var, const float* varValues) {
        return {
            IntRange { (int)varValues[var.left().id], (int)varValues[var.right().id] },
            IntRange { (int)varValues[var.bottom().id], (int)varValues[var.top().id] }
        };
    }

    Texture atlas;
    const int atlasPadding = 16;
    int nextAtlasAllocationHeight = 0;
    IntVec allocateTexture(IntVec dimensions);
    void deallocateTexture(IntRect region);

    std::vector<uint32_t> outputBlockMap;
    int_t outputLayers = 1;
    std::vector<IntVec> outputOffsets;

    // FIXME: should not be public
public:
    Texture framebuffer;
};
struct UiElement {
    Layout& m_layout;
    UiElement(Layout& lay)
        : m_layout(lay) { }
    Layout& layout() { return m_layout; }
    virtual void setStyle(BigStyle) { }
    virtual ~UiElement() { }
};

struct LayoutBox : UiElement {
    Rect padding;
    RectVar outerBox, innerBox;
    RectEqn paddingEqn;
    LayoutBox(Layout& lay)
        : UiElement(lay)
        , outerBox(layout().newRect())
        , innerBox(layout().newRect()) {
        paddingEqn = layout().align(innerBox, outerBox);
    }
    ~LayoutBox() {
        layout().removeEqn(paddingEqn);
        layout().deleteVariable(outerBox);
        layout().deleteVariable(innerBox);
    }
    void updatePaddings() { layout().updateInhom(paddingEqn, padding); }
    void setStyle(BigStyle) override { }
};

}