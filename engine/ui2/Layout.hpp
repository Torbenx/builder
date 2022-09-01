#pragma once

#include "Style.hpp"
#include "containers/IdAllocator.hpp"
#include "math/sparse/LUDecomposition.hpp"
#include "util/types.hpp"
#include <vector>

namespace ui {

struct Variable {
    uint32_t id = 0;
};
struct Box {
    uint32_t baseId = 0;
    Variable left() const { return { baseId }; }
    Variable right() const { return { baseId + 1 }; }
    Variable bottom() const { return { baseId + 2 }; }
    Variable top() const { return { baseId + 3 }; }
};
struct Equation {
    uint32_t id = 0;
};
struct BoxEqn {
    Equation left, right, bottom, top;
};
class Layout {
public:
    Layout(const Layout&) = delete;
    ~Layout();
    Variable newVar();
    Box newBox();
    Equation align(Variable leftBottom, Variable rightTop);
    BoxEqn align(Box inner, Box outer);
    void updateInhom(Equation, TypedSize inhom);
    void updateInhom(BoxEqn, Rect inhom);
    void removeEqn(Equation);
    void removeEqn(BoxEqn);
    void deleteVariable(Variable);
    void deleteBox(Box);

private:
    IdAllocator<uint32_t> eqnIdAlloc;
    IdAllocator<uint32_t> varIdAlloc;
    std::vector<TypedSize> inhomTyped;
    std::vector<float> inhomValues;
    sparse::Matrix mat;
};
struct UiElement {
    Layout& m_layout;
    UiElement(Layout& lay)
        : m_layout(lay) { }
    Layout& layout() { return m_layout; }
    virtual void setStyle(StyleView) { }
    virtual ~UiElement() { }
};

struct LayoutBoxStyle : StyleItem<LayoutBoxStyle, LayoutBoxStyle> {
    Rect padding;
};
struct LayoutBox : UiElement {
    LayoutBoxStyle localValues;
    LayoutBoxStyle baseStyle;
    Box outerBox, innerBox;
    BoxEqn paddingEqn;
    LayoutBox(Layout& lay)
        : UiElement(lay)
        , outerBox(layout().newBox())
        , innerBox(layout().newBox()) {
        paddingEqn = layout().align(innerBox, outerBox);
    }
    ~LayoutBox() {
        layout().removeEqn(paddingEqn);
        layout().deleteBox(outerBox);
        layout().deleteBox(innerBox);
    }
    void updatePaddings() { layout().updateInhom(paddingEqn, applyLocalValue(baseStyle.padding, localValues.padding)); }
    void setStyle(const LayoutBoxStyle& style) {
        baseStyle = style;
        updatePaddings();
    }
    void setStyle(StyleView style) override { setStyle(style.get<LayoutBoxStyle>()); }
};

}