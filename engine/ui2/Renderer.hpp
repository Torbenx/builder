#pragma once

#include "Rect.hpp"
#include "StyleConsumer.hpp"
#include "ZIndex.hpp"
#include "software/Image.hpp"

namespace ui {

inline constexpr gfx::TexelLayout IMAGE_FORMAT = gfx::TexelLayout::BGRA8;
using ImagePtr = sw::ImagePtr<IMAGE_FORMAT>;
using ImageView = sw::ImageView<IMAGE_FORMAT>;

class RendererBase;
struct RendererRecord {
    void (*updateStyleAndPrepareRender)(RendererBase*, void*) = nullptr;
    void (*prepareRender)(RendererBase*, void*) = nullptr;
    void (*render)(ImageView, const void*) = nullptr;
    int_t renderDataSizeAlign8 = 0;
};

}

DECLARE_RECORDED_CLASS(const ui::RendererRecord, ui_renderers);

namespace ui {

class RendererBase {
public:
    static int_t recordCount() { return records::ui_renderers().size(); }
    static const RendererRecord& record(int_t index) { return records::ui_renderers()[index]; }

    uint32_t id = 0;

protected:
    void invalidateStyleInternal(int_t index);
};
struct RendererHandle;

template<typename Impl, typename S, typename R = S>
class Renderer : public RendererBase {
public:
    using Style = S;
    using RData = R;
    friend struct RendererHandle;

private:
    static void updateStyleAndPrepareRender(RendererBase* base, void* renderDataOut) {
        Renderer* thiz = (Renderer*)base;
        RData* renderData = (RData*)renderDataOut;
        thiz->isDirty = false;
        thiz->m_computedStyle = computeStyle(thiz->bigTable, thiz->baseStyle, thiz->localStyle);
        thiz->impl()->applyStyle(thiz->m_computedStyle);
        thiz->impl()->getRenderData(*renderData);
    }
    static void prepareRender(RendererBase* base, void* renderDataOut) {
        Renderer* thiz = (Renderer*)base;
        RData* renderData = (RData*)renderDataOut;
        thiz->impl()->getRenderData(*renderData);
    }
    static void dispatchRender(ImageView image, const void* renderDataIn) {
        const RData* renderData = (const RData*)renderDataIn;
        Impl::render(image, *renderData);
    }

    static constexpr RendererRecord makeRecord() {
        static_assert(alignof(RData) <= 8);
        return {
            .updateStyleAndPrepareRender = updateStyleAndPrepareRender,
            .prepareRender = prepareRender,
            .render = dispatchRender,
            .renderDataSizeAlign8 = stx::alignmentCeil(8, sizeof(RData)),
        };
    }
    RECORDED_CLASS_IMPL(const ui::RendererRecord, ui_renderers, makeRecord);

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
        invalidateStyleInternal(classIndex());
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
concept IsRenderer = std::derived_from<T, Renderer<T, typename T::Style, typename T::RData>>;

struct RendererHandle {
    RendererBase* base = nullptr;
    int_t classIndex = -1;
    RendererHandle() = default;
    template<IsRenderer T>
    RendererHandle(T& r)
        : base(&r), classIndex(T::classIndex()) { }
    RendererBase* operator->() const { return base; }
    const RendererRecord& record() {
        return RendererBase::record(classIndex);
    }
    void prepareRender(void* renderData) {
        record().prepareRender(base, renderData);
    }
};

void draw(RendererHandle, RectVarHandle);
void cancelDraw(RendererHandle);

}