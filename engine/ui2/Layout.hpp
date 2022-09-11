#pragma once

#include "Style.hpp"
#include "Size.hpp"
#include "containers/IdAllocator.hpp"
#include "math/sparse/Vector.hpp"
#include "util/types.hpp"
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
    uint32_t* data = nullptr;
    int_t stride = 0;
    int_t width = 0;
    int_t height = 0;
};
struct TextureCallback {
    virtual void onTextureChange(TextureView) = 0;
};

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

    void doLayout(int_t width, int_t height, float ptScale);

    RectVar screen() const { return screenBox; }
    void debug(RectVar box) const {
        log("Box: {}, {} - {}, {}", varValues[box.left().id], varValues[box.bottom().id], varValues[box.right().id], varValues[box.top().id]);
    }

    void requestTexture(RectVar, TextureCallback*);
    void cancelTexture(TextureCallback*);

private:
    IdAllocator<uint32_t> eqnIdAlloc;
    uint32_t nextVarId = 4;
    std::vector<Size> inhomTyped;
    std::vector<float> inhomValues;
    std::vector<float> varValues;
    sparse::Matrix matrix;
    RectVar screenBox;
    RectEqn screenEqn;

    uint32_t allocateVariableIds(int_t count);
    Equation makeEquation(std::initializer_list<sparse::Entry>, Size inhom = {});
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