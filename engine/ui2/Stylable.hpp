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

// Layout Stylable
class LayoutStylableBase;
struct LayoutStylableRecord {
    void (*updateStyle)(LayoutStylableBase*) = nullptr;
};

}
DEFINE_SECTION(const ui::LayoutStylableRecord, layout_stylables);

namespace ui {

class LayoutStylableBase {
public:
    static int_t stylableCount() { return SECTION_ELEMENT_COUNT(layout_stylables); }
    static const LayoutStylableRecord& record(int_t index) { return __begin_layout_stylables[index]; }

protected:
    void invalidateStyleInternal(int_t index);
};
template<typename Impl, typename S>
class LayoutStylable : LayoutStylableBase {
public:
    using Style = S;

private:
    static void updateStyle(LayoutStylableBase* base) {
        LayoutStylable* thiz = (LayoutStylable*)base;
        thiz->isDirty = false;
        thiz->m_computedStyle = computeStyle(thiz->bigTable, thiz->baseStyle, thiz->localStyle);
        thiz->impl()->applyStyle(thiz->m_computedStyle);
    }

    static constexpr IN_SECTION(layout_stylables) LayoutStylableRecord record = {
        .updateStyle = updateStyle,
    };
    static int_t index() { return &record - __begin_layout_stylables; }
    Impl* impl() { return static_cast<Impl*>(this); }
    bool isDirty = false;
    BigStyleTable bigTable;
    Ptr<Style> baseStyle;
    Ref<Style> localStyle;
    Style m_computedStyle;

    void invalidateStyle() {
        if (isDirty)
            return;
        isDirty = true;
        invalidateStyleInternal(index());
    }

public:
    const Style& computedStyle() const { return m_computedStyle; }
    const Ref<Style>& local() const { return localStyle; }
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
};

// Render Stylable
class RenderStylableBase;
struct RenderStylableRecord {
    void (*updateStyleAndPrepareRender)(RenderStylableBase*, void*) = nullptr;
    void (*prepareRender)(RenderStylableBase*, void*) = nullptr;
    void (*render)(TextureState, const void*) = nullptr;
    int_t renderDataSizeAlign8 = 0;
};

}

DEFINE_SECTION(const ui::RenderStylableRecord, render_stylables);

namespace ui {

class RenderStylableBase {
public:
    static int_t stylableCount() { return SECTION_ELEMENT_COUNT(render_stylables); }
    static const RenderStylableRecord& record(int_t index) { return __begin_render_stylables[index]; }

    uint32_t id = 0;

protected:
    void invalidateStyleInternal(int_t index);
};

template<typename Impl, typename S, typename R = S>
class RenderStylable : public RenderStylableBase {
public:
    using Style = S;
    using RData = R;

    static int_t stylableIndex() { return &record - __begin_render_stylables; }

private:
    static void updateStyleAndPrepareRender(RenderStylableBase* base, void* renderDataOut) {
        RenderStylable* thiz = (RenderStylable*)base;
        RData* renderData = (RData*)renderDataOut;
        thiz->isDirty = false;
        thiz->m_computedStyle = computeStyle(thiz->bigTable, thiz->baseStyle, thiz->localStyle);
        thiz->impl()->applyStyle(thiz->m_computedStyle);
        thiz->impl()->getRenderData(*renderData);
    }
    static void prepareRender(RenderStylableBase* base, void* renderDataOut) {
        RenderStylable* thiz = (RenderStylable*)base;
        RData* renderData = (RData*)renderDataOut;
        thiz->impl()->getRenderData(*renderData);
    }
    static void dispatchRender(TextureState texture, const void* renderDataIn) {
        const RData* renderData = (const RData*)renderDataIn;
        Impl::render(texture, *renderData);
    }

    static_assert(alignof(RData) <= 8);
    static constexpr IN_SECTION(render_stylables) RenderStylableRecord record = {
        .updateStyleAndPrepareRender = updateStyleAndPrepareRender,
        .prepareRender = prepareRender,
        .render = dispatchRender,
        .renderDataSizeAlign8 = stx::alignmentCeil(8, sizeof(RData)),
    };
    Impl* impl() { return static_cast<Impl*>(this); }
    bool isDirty = false;
    BigStyleTable bigTable;
    Ptr<Style> baseStyle;
    Ref<Style> localStyle;
    Style m_computedStyle;

    void getRenderData(RData& out) requires std::is_same_v<RData, Style> {
        out = computedStyle();
    }
    void applyStyle(const Style&) { }

    void invalidateStyle() {
        if (isDirty)
            return;
        isDirty = true;
        invalidateStyleInternal(stylableIndex());
    }

public:
    const Style& computedStyle() const { return m_computedStyle; }
    const Ref<Style>& local() const { return localStyle; }
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
};

template<typename T>
concept Renderer = std::derived_from<T, RenderStylable<T, typename T::Style, typename T::RData>>;

struct RendererHandle {
    RenderStylableBase* base = nullptr;
    int_t stylableIndex = -1;
    RendererHandle() = default;
    template<Renderer T>
    RendererHandle(T& r)
        : base(&r), stylableIndex(T::stylableIndex()) { }
    RenderStylableBase* operator->() const { return base; }
    const RenderStylableRecord& record() {
        return RenderStylableBase::record(stylableIndex);
    }
    void prepareRender(void* renderData) {
        record().prepareRender(base, renderData);
    }
};

void draw(RendererHandle, RectVarHandle);
void cancelDraw(RendererHandle);

}