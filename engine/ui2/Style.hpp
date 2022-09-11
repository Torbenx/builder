#pragma once

#include "Size.hpp"
#include "memory/TagPtr.hpp"
#include "util/target.hpp"

namespace ui {
struct StyleKeyRecord {
    int_t styleRefBytes = 0;
    int_t typeBytes = 0;
};
}

DEFINE_SECTION(ui::StyleKeyRecord, ui_style_keys)

namespace ui {

template<typename Key>
struct SmallStyleStorage {
    typename Key::Type value;
};
template<typename... Keys>
struct SmallStyleImpl : SmallStyleStorage<Keys>... { };

template<typename... Keys>
struct SmallStyleAccumulator {
    using type = SmallStyleImpl<Keys...>;
};
template<typename... Keys1, typename... Keys2>
struct SmallStyleAccumulator<SmallStyleImpl<Keys1...>, Keys2...> {
    using type = typename SmallStyleAccumulator<Keys2..., Keys1...>::type;
};
template<typename... Keys>
using SmallStyle = typename SmallStyleAccumulator<Keys...>::type;

template<typename Key>
auto& get(SmallStyleStorage<Key>& storage) {
    return storage.value;
}
template<typename Key>
const auto& get(const SmallStyleStorage<Key>& storage) {
    return storage.value;
}
template<typename Callback, typename... Keys>
void forEachKey(SmallStyleImpl<Keys...>& style, Callback&& callback) {
    (callback.template operator()<Keys>(get<Keys>(style)), ...);
}

struct BigStyle {
    const void* const* data = nullptr;
};
template<typename Key>
const auto& get(BigStyle style) {
    return *(const typename Key::Type*)style.data[Key::styleKey()];
}

struct RefInfo {
    uint32_t styleKey = 0;
    uint32_t byteOffset = 0;
};
struct StyleRefPtr {
    uintptr_t data = 0;
    bool isRef() const { return data & 1; }
    bool isValue() const { return !isRef(); }
    uintptr_t highPart() const { return (data & ~(uintptr_t)1); }
    void* value() const { return (void*)highPart(); }
    void set(void* value) { data = (uintptr_t)value; }
    void set(RefInfo info) {
        data = ((uintptr_t)info.styleKey << 1) | ((uintptr_t)info.byteOffset << 33) | (uintptr_t)1;
    }
    RefInfo ref() const {
        uintptr_t high = highPart();
        return { .styleKey = static_cast<uint32_t>(high >> 1), .byteOffset = static_cast<uint32_t>(high >> 33) };
    }
};

struct MakeSelfReference {
    const std::byte* baseAddress = nullptr;
    uint32_t styleKey = 0;
    MakeSelfReference(const void* baseAddress, int_t styleKey)
        : baseAddress((const std::byte*)baseAddress), styleKey(styleKey) { }
    RefInfo operator()(const void* thiz) const {
        return { .styleKey = styleKey, .byteOffset = (uint32_t)((const std::byte*)thiz - baseAddress) };
    }
};

template<typename T>
struct StyleRef;

template<>
struct StyleRef<BigStyle> {
    static StyleRef<BigStyle> allocate() {
        int_t keyCount = __end_ui_style_keys - __begin_ui_style_keys;
        log<Tag::Ui>("found {} style keys", keyCount);
        void** data = new void*[keyCount]();
        for (int_t i = 0; i < keyCount; i++) {
            const StyleKeyRecord& record = __begin_ui_style_keys[i];
            data[i] = new std::byte[record.styleRefBytes]();
        }
        return { data };
    }
    void* const* data = nullptr;
    void* fetch(RefInfo ref) const {
        std::byte* base = (std::byte*)data[ref.styleKey];
        return base + ref.byteOffset;
    }
};
template<typename Key>
StyleRef<typename Key::Type>& get(StyleRef<BigStyle> style) {
    return *(StyleRef<typename Key::Type>*)style.data[Key::styleKey()];
}
template<typename Key>
typename StyleRef<typename Key::Type>::ValueType resolve(StyleRef<BigStyle> style) {
    auto& value = get<Key>(style);
    return value.resolve(style);
}

template<typename T>
struct StyleRef : StyleRefPtr {
    using ValueType = T*;
    StyleRef() { }
    StyleRef(const MakeSelfReference& selfRef) { set(selfRef(this)); }
    T* value() const { return (T*)StyleRefPtr::value(); }
    T* resolve(StyleRef<BigStyle> style) const {
        StyleRef<T> current = *this;
        while (current.isRef())
            current = *(StyleRef<T>*)style.fetch(ref());
        return current.value();
    }
};
template<>
struct StyleRef<Size> {
    using ValueType = Size;
    uint32_t lowPart = 0;
    uint32_t highPart = 0;
    StyleRef() { }
    StyleRef(const MakeSelfReference& selfRef) { set(selfRef(this)); }
    StyleRef(const Size& value)
        : lowPart(0b10), highPart(std::bit_cast<uint32_t>(value)) { }
    bool isRef() const { return lowPart & 1; }
    bool isValue() const { return !isRef(); }
    Size value() const { return std::bit_cast<Size>(highPart); }

    RefInfo ref() const { return { .styleKey = highPart, .byteOffset = lowPart >> 2 }; }
    void set(RefInfo info) {
        highPart = info.styleKey;
        lowPart = (info.byteOffset << 2) | (uint32_t)1;
    }
    Size resolve(StyleRef<BigStyle> style) const {
        StyleRef<Size> current = *this;
        while (current.isRef())
            current = *(StyleRef<Size>*)style.fetch(ref());
        return current.value();
    }
};
template<>
struct StyleRef<Range> {
    using ValueType = Range;
    StyleRef<Size> m_low;
    StyleRef<Size> m_high;
    StyleRef() { }
    StyleRef(const MakeSelfReference& selfRef)
        : m_low(selfRef), m_high(selfRef) { }
    StyleRef(const Range& range)
        : m_low(range.low()), m_high(range.high()) { }
    StyleRef(const StyleRef<Size>& value)
        : StyleRef(value, value) { }
    StyleRef(const StyleRef<Size>& low, const StyleRef<Size>& high)
        : m_low(low), m_high(high) { }
    StyleRef<Size>& low() { return m_low; }
    const StyleRef<Size>& low() const { return m_low; }
    StyleRef<Size>& high() { return m_high; }
    const StyleRef<Size>& high() const { return m_high; }
    Range value() const { return { low().value(), high().value() }; }
    Range resolve(StyleRef<BigStyle> style) const { return { low().resolve(style), high().resolve(style) }; }
};
template<>
struct StyleRef<Rect> {
    using ValueType = Rect;
    StyleRef<Range> m_x;
    StyleRef<Range> m_y;
    StyleRef() { }
    StyleRef(const MakeSelfReference& selfRef)
        : m_x(selfRef), m_y(selfRef) { }
    StyleRef(const Rect& rect)
        : m_x(rect.x()), m_y(rect.y()) { }
    StyleRef(const StyleRef<Size>& value)
        : StyleRef(StyleRef<Range>(value)) { }
    StyleRef(const StyleRef<Range>& range)
        : StyleRef(range, range) { }
    StyleRef(const StyleRef<Range>& x, const StyleRef<Range>& y)
        : m_x(x), m_y(y) { }
    StyleRef<Range>& x() { return m_x; }
    const StyleRef<Range>& x() const { return m_x; }
    StyleRef<Range>& y() { return m_y; }
    const StyleRef<Range>& y() const { return m_y; }
    StyleRef<Range>& axis(Axis axis) { return axis == Axis::X ? x() : y(); }
    const StyleRef<Range>& axis(Axis axis) const { return axis == Axis::X ? x() : y(); }
    StyleRef<Size>& left() { return x().low(); }
    const StyleRef<Size>& left() const { return x().low(); }
    StyleRef<Size>& right() { return x().high(); }
    const StyleRef<Size>& right() const { return x().high(); }
    StyleRef<Size>& bottom() { return y().low(); }
    const StyleRef<Size>& bottom() const { return y().low(); }
    StyleRef<Size>& top() { return y().high(); }
    const StyleRef<Size>& top() const { return y().high(); }
    Rect value() const { return { x().value(), y().value() }; }
    Rect resolve(StyleRef<BigStyle> style) const { return { x().resolve(style), y().resolve(style) }; }
};

template<typename Key>
struct SmallStyleRefStorage {
    StyleRef<typename Key::Type> value;
    SmallStyleRefStorage() = default;
    SmallStyleRefStorage(const MakeSelfReference& selfRef)
        : value(selfRef) { }
};
template<typename... Keys>
struct StyleRef<SmallStyleImpl<Keys...>> : SmallStyleRefStorage<Keys>... {
    using ValueType = SmallStyleImpl<Keys...>;
    StyleRef() = default;
    StyleRef(const MakeSelfReference& selfRef)
        : SmallStyleRefStorage<Keys>(selfRef)... { }
    ValueType resolve(StyleRef<BigStyle> style) const {
        return { { get<Keys>(*this).resolve(style) }... };
    }
};
template<typename Key>
auto& get(SmallStyleRefStorage<Key>& storage) {
    return storage.value;
}
template<typename Key>
const auto& get(const SmallStyleRefStorage<Key>& storage) {
    return storage.value;
}

template<typename Key, typename T>
struct StyleKey {
    using Type = T;
    static constinit inline IN_SECTION(ui_style_keys) const StyleKeyRecord keyRecord = {
        .styleRefBytes = sizeof(StyleRef<Type>),
        .typeBytes = sizeof(Type),
    };
    static int_t styleKey() {
        static_assert(std::is_base_of_v<StyleKey, Key>);
        return &keyRecord - __begin_ui_style_keys;
    }
    static const inline StyleRef<Type> selfRef = StyleRef<Type>(MakeSelfReference(&selfRef, styleKey()));
};
template<typename Key>
const auto& ref() { return Key::selfRef; }

struct Padding : StyleKey<Padding, Rect> { };
struct TextSize : StyleKey<TextSize, Size> { };
using TextBoxStyle = SmallStyle<Padding, TextSize>;
struct NormalTextBox : StyleKey<NormalTextBox, TextBoxStyle> { };

}