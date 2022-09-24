#pragma once

#include "BigStyleTable.hpp"
#include "SmallStyleTable.hpp"
#include "StyleKey.hpp"
#include "event/InlineCallback.hpp"
#include "handles.hpp"
#include "render.hpp"

namespace ui {

struct Resolver {
    Ref<BigStyleTable> table;
};
template<typename T>
T resolveLeaf(const Ref<T>& in, const Resolver& resolver, const T& base) {
    Ref<T> current = in;
    while (current.isPtr()) {
        Ptr<T> ptr = current.ptr();
        if (!ptr)
            return base;
        current = resolver.table.get(ptr);
    }
    return current.value();
}

struct Substituter {
    BigStyleTable table;
};
template<typename T>
T substituteLeaf(const Ref<T>& in, const Substituter& subs, const T& base) {
    if (!in.isPtr())
        return in.value();
    Ptr<T> ptr = in.ptr();
    if (!ptr)
        return base;
    return subs.table.get(ptr);
}

template<typename T>
T computeStyle(BigStyleTable bigTable, const Key<T>& baseStyle, const Ref<T>& localStyle) {
    return localStyle.substitute({ bigTable }, bigTable.get(baseStyle));
}

template<typename S>
class Stylable {
public:
    using Style = S;

    Stylable(evt::Signal onStyleDirty)
        : onStyleDirty(onStyleDirty) { }
    void setTable(BigStyleTable table) {
        bigTable = table;
       // onStyleDirty.notify();
    }
    void setStyle(Ptr<Style> style) {
        baseStyle = style;
        //onStyleDirty.notify();
    }
    Ref<Style>& mutateLocal() {
        //onStyleDirty.notify();
        return localStyle;
    }
    const Ref<Style>& local() const { return localStyle; }
    Ptr<Style> style() const { return baseStyle; }

protected:
    const Style& computedStyle() const { return m_computedStyle; }

private:
    BigStyleTable bigTable;
    Ptr<Style> baseStyle;
    Ref<Style> localStyle;
    Style m_computedStyle;
    evt::Signal onStyleDirty;
};
template<typename T>
concept IsStylable = requires(T& t, BigStyleTable table, Ptr<typename T::Style> ptr) {
    t.setTable(table);
    t.setStyle(ptr);
};

template<IsStylable... Ts>
struct CompoundStylable : Ts... {
    using Style = SmallStyleTable<typename Ts::Style...>;
    template<typename T>
    T& get() { return *(T*)this; }
    void setTable(BigStyleTable table) {
        (get<Ts>().setTable(table), ...);
    }
    void setStyle(Ptr<Style> style) {
        (get<Ts>().setStyle(style), ...);
    }
};

template<typename Impl, typename Style>
struct BasicRenderer : RendererBase, evt::InlineCallback, Stylable<Style> {
    // impl has render(const TextureState& state, const Style& style)
    Impl* impl() { return static_cast<Impl*>(this); }
    void do_render() {
        impl()->render(this->textureState(), this->computedStyle());
    }
    BasicRenderer()
        : evt::InlineCallback(stx::constant<&BasicRenderer::do_render>())
        , Stylable<Style>(this->inlineCallback()) { }
    evt::Signal textureUpdateSignal() const { return this->inlineCallback(); }
};

DECLARE_STYLE_KEY(TextSize, Size);
using TextStyle = SmallStyleTable<TextSize>;

struct TextRenderer : BasicRenderer<TextRenderer, TextStyle> {
    void render(const TextureState&, const Style&) {
    }
};

}