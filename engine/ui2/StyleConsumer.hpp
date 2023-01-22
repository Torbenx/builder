#pragma once

#include "BigStyleTable.hpp"
#include "SmallStyleTable.hpp"
#include "StyleKey.hpp"
#include "handles.hpp"

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
    if (!in.isPtr()) {
        if constexpr (std::is_same_v<T, Size>)
            log("had local value: {}", in.value().value());
        return in.value();
    }
    Ptr<T> ptr = in.ptr();
    if (ptr) {
        if constexpr (std::is_same_v<T, Size>)
            log("had local ptr, resolved to: {}", subs.table.get(ptr).value());
        return subs.table.get(ptr);
    }
    if constexpr (std::is_same_v<T, Size>)
        log("fallback to global value {}", base.value());
    return base;
}

template<typename T>
T computeStyle(BigStyleTable bigTable, Ptr<T> baseStyle, const Ref<T>& localStyle) {
    return localStyle.substitute({ bigTable }, bigTable.get(baseStyle));
}

class LayoutMetadata {
private:
    float m_ptScale = 1.0f;

public:
    LayoutMetadata(float ptScale)
        : m_ptScale(ptScale) { }
    float ptScale() const { return m_ptScale; }
};

class StyleConsumerBase;
struct StyleConsumerRecord {
    void (*updateStyle)(StyleConsumerBase*, LayoutMetadata) = nullptr;
};

}
DECLARE_RECORDED_CLASS(const ui::StyleConsumerRecord, ui_style_consumers);

namespace ui {

class StyleConsumerBase {
public:
    static int_t recordCount() { return records::ui_style_consumers().size(); }
    static const StyleConsumerRecord& record(int_t index) { return records::ui_style_consumers()[index]; }

protected:
    void invalidateStyleInternal(int_t index);
};
template<typename Impl, typename S>
class StyleConsumer : StyleConsumerBase {
public:
    using Style = S;

private:
    static void updateStyle(StyleConsumerBase* base, LayoutMetadata meta) {
        StyleConsumer* thiz = (StyleConsumer*)base;
        thiz->isDirty = false;
        thiz->m_computedStyle = computeStyle(thiz->bigTable, thiz->baseStyle, thiz->localStyle);
        thiz->impl()->applyStyle(thiz->m_computedStyle, meta);
    }

    static constexpr StyleConsumerRecord makeRecord() {
        return { .updateStyle = updateStyle };
    }
    RECORDED_CLASS_IMPL(const ui::StyleConsumerRecord, ui_style_consumers, makeRecord);
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
    template<ContainedIn<Style> Key>
    auto& mut() { return mutateLocal().template get<Key>(); }
};

}