#pragma once

#include "Rect.hpp"
#include "ZIndex.hpp"
#include "StyleConsumer.hpp"
#include "event/Script.hpp"

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
struct TextureState {
    TextureView texture;
    IntVec dimensions;
};

class RendererBase;
struct RendererRecord {
    void (*updateStyleAndPrepareRender)(RendererBase*, void*) = nullptr;
    void (*prepareRender)(RendererBase*, void*) = nullptr;
    void (*render)(TextureState, const void*) = nullptr;
    int_t renderDataSizeAlign8 = 0;
};

}

DEFINE_SECTION(const ui::RendererRecord, renderer_records);

namespace ui {

class RendererBase {
public:
    static int_t recordCount() { return SECTION_ELEMENT_COUNT(renderer_records); }
    static const RendererRecord& record(int_t index) { return __begin_renderer_records[index]; }

    uint32_t id = 0;

protected:
    void invalidateStyleInternal(int_t index);
};

template<typename Impl, typename S, typename R = S>
class Renderer : public RendererBase {
public:
    using Style = S;
    using RData = R;

    static int_t classIndex() { return &record - __begin_renderer_records; }

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
    static void dispatchRender(TextureState texture, const void* renderDataIn) {
        const RData* renderData = (const RData*)renderDataIn;
        Impl::render(texture, *renderData);
    }

    static_assert(alignof(RData) <= 8);
    static constexpr IN_SECTION(renderer_records) RendererRecord record = {
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