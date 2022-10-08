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
concept Stylable = requires(T& t, BigStyleTable table, Ptr<typename T::Style> ptr) {
    t.setTable(table);
    t.setStyle(ptr);
};

template<Stylable... Ts>
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

class StyleConsumerBase;
struct StyleConsumerRecord {
    void (*updateStyle)(StyleConsumerBase*) = nullptr;
};

}
DEFINE_SECTION(const ui::StyleConsumerRecord, style_consumer_records);

namespace ui {

class StyleConsumerBase {
public:
    static int_t recordCount() { return SECTION_ELEMENT_COUNT(style_consumer_records); }
    static const StyleConsumerRecord& record(int_t index) { return __begin_style_consumer_records[index]; }

protected:
    void invalidateStyleInternal(int_t index);
};
template<typename Impl, typename S>
class StyleConsumer : StyleConsumerBase {
public:
    using Style = S;

private:
    static void updateStyle(StyleConsumerBase* base) {
        StyleConsumer* thiz = (StyleConsumer*)base;
        thiz->isDirty = false;
        thiz->m_computedStyle = computeStyle(thiz->bigTable, thiz->baseStyle, thiz->localStyle);
        thiz->impl()->applyStyle(thiz->m_computedStyle);
    }

    static constexpr IN_SECTION(style_consumer_records) StyleConsumerRecord record = {
        .updateStyle = updateStyle,
    };
    static int_t classIndex() { return &record - __begin_style_consumer_records; }
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
};

}