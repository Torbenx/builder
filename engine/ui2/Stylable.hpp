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
T resolveLeaf(const Ref<T>& in, const Resolver& resolver) {
    Ref<T> current = in;
    while (current.isPtr()) {
        Ptr<T> ptr = current.ptr();
        VERIFY(Ui, (bool)ptr);
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
    if (ptr)
        return subs.table.get(ptr);
    return base;
}

template<typename T>
T computeStyle(BigStyleTable bigTable, Ptr<T> baseStyle, const Ref<T>& localStyle) {
    return localStyle.substitute({ bigTable }, bigTable.get(baseStyle));
}

struct StylableBase {
    BigStyleTable bigTable;
    evt::Signal onStyleDirty;
    bool isDirty = false;
    StylableBase(evt::Signal onStyleDirty)
        : onStyleDirty(onStyleDirty) { }
    void invalidateStyleInternal(void (*)(StylableBase*));
};

template<typename S>
class Stylable : private StylableBase {
public:
    using Style = S;

    using StylableBase::StylableBase;

    void setTable(BigStyleTable table) {
        invalidateStyle();
        bigTable = table;
    }
    void setStyle(Ptr<Style> style) {
        invalidateStyle();
        baseStyle = style;
    }
    Ref<Style>& mutateLocal() {
        invalidateStyle();
        return localStyle;
    }
    const Ref<Style>& local() const { return localStyle; }
    Ptr<Style> style() const { return baseStyle; }

    void invalidateStyle() {
        if (isDirty)
            return;
        isDirty = true;
        StylableBase::invalidateStyleInternal([](StylableBase* base) {
            Stylable* thiz = (Stylable*)base;
            thiz->m_computedStyle = computeStyle(thiz->bigTable, thiz->baseStyle, thiz->localStyle);
        });
    }

protected:
    const Style& computedStyle() const { return m_computedStyle; }

private:
    Ptr<Style> baseStyle;
    Ref<Style> localStyle;
    Style m_computedStyle;
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
    template<typename T>
    const T& get() const { return *(const T*)this; }
    void setTable(BigStyleTable table) {
        (get<Ts>().setTable(table), ...);
    }
    void setStyle(Ptr<Style> style) {
        (get<Ts>().setStyle(style), ...);
    }
};

template<typename Impl, typename Style>
struct BasicRenderer : RendererBase, evt::InlineCallback, Stylable<Style> {
    static constexpr evt::CompiledCondition updateCondition = []() {
        return "textureUpdate"_ev || "styleUpdate"_ev;
    }();
    // impl has render(const TextureState& state, const Style& style)
    Impl* impl() { return static_cast<Impl*>(this); }
    void do_render() {
        impl()->render(this->textureState(), this->computedStyle());
    }
    BasicRenderer()
        : evt::InlineCallback(stx::constant<&BasicRenderer::do_render>(), updateCondition)
        , Stylable<Style>(updateCondition.signal(this->inlineScript(), "styleUpdate"_ev)) {
        this->inlineScript()->executor = renderingExecutor();
    }
    evt::Signal textureUpdateSignal() const {
        return updateCondition.signal(this->inlineScript(), "textureUpdate"_ev);
    }
};

template<typename Impl, typename Style>
struct BasicStylable : evt::InlineCallback, Stylable<Style> {
    Impl* impl() { return static_cast<Impl*>(this); }
    void do_applyStyle() {
        impl()->applyStyle(this->computedStyle());
    }
    BasicStylable()
        : evt::InlineCallback(stx::constant<&BasicStylable::do_applyStyle>())
        , Stylable<Style>(evt::monoCondition.signal(this->inlineScript())) {
        this->inlineScript()->executor = stylingExecutor();
    }
};

}