#pragma once

#include "Style.hpp"
#include "containers/IdAllocator.hpp"
#include "math/sparse/Vector.hpp"
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
    Box newBox();
    Equation align(Variable leftBottom, Variable rightTop, TypedSize inhom = {});
    BoxEqn align(Box inner, Box outer, Rect inhom = {});
    void updateInhom(Equation, TypedSize inhom = {});
    void updateInhom(BoxEqn, Rect inhom = {});
    void removeEqn(Equation);
    void removeEqn(BoxEqn);
    void deleteVariable(Variable);
    void deleteBox(Box);

    ScreenMetrics metrics() const { return m_metrics; }
    void doLayout(ScreenMetrics newMetrics);
    Box screen() const { return screenBox; }
    void debug(Box box) const {
        log("Box: {}, {} - {}, {}", varValues[box.left().id], varValues[box.bottom().id], varValues[box.right().id], varValues[box.top().id]);
    }

    void requestTexture(Box, TextureCallback*);
    void cancelTexture(TextureCallback*);

private:
    IdAllocator<uint32_t> eqnIdAlloc;
    uint32_t nextVarId = 4;
    std::vector<TypedSize> inhomTyped;
    std::vector<float> inhomValues;
    std::vector<float> varValues;
    sparse::Matrix matrix;
    ScreenMetrics m_metrics;
    Box screenBox;
    BoxEqn screenEqn;

    uint32_t allocateVariableIds(int_t count);
    void resolveTypedSizes();
    Equation makeEquation(std::initializer_list<sparse::Entry>, TypedSize inhom = {});
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
    LayoutBoxStyle localValues { .padding = Rect::noValues() };
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