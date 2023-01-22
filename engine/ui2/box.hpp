#pragma once

#include "Renderer.hpp"
#include "equation_code.hpp"


namespace ui {

template<typename T, typename Style>
concept StyledBoxImpl = std::is_same_v<typename T::Style, Style> && requires(T impl) {
    { impl.mutate() } -> std::same_as<Ref<Style>&>;
};
template<typename B>
concept StyledBox = requires(B box, BigStyleTable table, Ptr<typename B::Style> ptr) {
    box.setTable(table);
    box.setStyle(ptr);
    { box.box() } -> std::same_as<RectVarHandle>;
};

template<IsRenderer R>
struct ContentBox : R {
    RectVar contentBox;
    RectVarHandle box() const { return contentBox; }
    ContentBox() {
        draw((R&)*this, contentBox);
    }
    ~ContentBox() {
        cancelDraw((R&)*this);
    }
    R& renderer() { return *this; }
    const R& renderer() const { return *this; }
};

DECLARE_STYLE_KEY(Padding, Rect);
using PaddingStyle = SmallStyleTable<Padding>;
struct PaddedImpl : StyleConsumer<PaddedImpl, PaddingStyle> {
    RectVar paddedBox;
    RectEqn paddingEqn;
    PaddedImpl(RectVarHandle innerBox) {
        using namespace code;
        paddingEqn = paddedBox == extendBy(innerBox, {});
    }
    void applyStyle(const PaddingStyle& style, LayoutMetadata) {
        paddingEqn.updateShift(style.get<Padding>());
    }
};
template<StyledBox B>
struct Padded : B, PaddedImpl {
    using Style = SmallStyleTable<typename B::Style, PaddingStyle>;
    Padded()
        : PaddedImpl { B::box() } { }
    RectVarHandle box() const { return PaddedImpl::paddedBox; }
    void setStyle(Ptr<Style> style) {
        B::setStyle(style);
        PaddedImpl::setStyle(style);
    }
    void setTable(BigStyleTable table) {
        B::setTable(table);
        PaddedImpl::setTable(table);
    }
    PaddedImpl& padded() { return *this; }
    const PaddedImpl& padded() const { return *this; }
    
    using B::mut;
    using PaddedImpl::mut;
};

}