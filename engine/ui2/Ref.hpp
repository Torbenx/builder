#pragma once

#include "Size.hpp"
#include "util/types.hpp"
#include <bit>

namespace ui {

struct Resolver;

struct RefInfo {
    static constexpr int COMPACT_OFFSET_BITS = 12;
    static constexpr uint32_t MIN_OFFSET_ALIGNMENT = 4;
    static constexpr uint32_t MAX_OFFSET = ((size_t)1 << COMPACT_OFFSET_BITS) * MIN_OFFSET_ALIGNMENT;
    static constexpr int COMPACT_KEY_BITS = 30 - COMPACT_OFFSET_BITS;
    static constexpr uint32_t MAX_KEY = (size_t)1 << COMPACT_KEY_BITS;
    static RefInfo fromCompact(uint32_t comp) {
        return {
            .styleKey = comp >> COMPACT_OFFSET_BITS,
            .byteOffset = (comp & (((uint32_t)1 << COMPACT_KEY_BITS) - 1)) * MIN_OFFSET_ALIGNMENT
        };
    }
    uint32_t compact() const {
        VERIFY(Ui, styleKey < MAX_KEY);
        VERIFY(Ui, stx::isAligned(MIN_OFFSET_ALIGNMENT, byteOffset));
        VERIFY(Ui, byteOffset < MAX_OFFSET);
        return (styleKey << COMPACT_OFFSET_BITS) | (byteOffset / MIN_OFFSET_ALIGNMENT);
    }
    uint32_t styleKey = 0;
    uint32_t byteOffset = 0;
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
struct Ref;

struct RefPtrBase {
    uintptr_t data = 0;
    bool isRef() const { return data & 1; }
    bool isValue() const { return !isRef(); }
    void set(void* value) {
        data = (uintptr_t)value;
    }
    void* value() const {
        return (void*)(data & ~(uintptr_t)1);
    }
    void set(RefInfo info) {
        data = ((uintptr_t)info.compact() << 32) | (uintptr_t)1;
    }
    RefInfo ref() const {
        return RefInfo::fromCompact(data >> 32);
    }
};
template<typename T>
struct Ref<T*> : RefPtrBase {
    Ref() { }
    Ref(const MakeSelfReference& selfRef) { set(selfRef(this)); }
    T* value() const { return (T*)RefPtrBase::value(); }
    T* resolve(const Resolver&) const;
};

template<>
struct Ref<Size> {
    Size data;
    static_assert(Size::payloadU32 == (uint32_t)1 << 30);
    Ref() { }
    Ref(const MakeSelfReference& selfRef) { set(selfRef(this)); }
    Ref(Size value) { set(value); }
    bool isRef() const { return data.payloadBit(); }
    bool isValue() const { return !isRef(); }
    void set(Size size) {
        VERIFY(Ui, !size.payloadBit());
        data = size;
    }
    Size value() const {
        return data.withoutPayload();
    }
    void set(RefInfo info) {
        data = Size::fromUint(info.compact() | Size::payloadU32);
    }
    RefInfo ref() const {
        return RefInfo::fromCompact(data.uint() & ~Size::payloadU32);
    }
    Size resolve(const Resolver&) const;
};

template<typename T>
requires(sizeof(T) == 1) struct Ref<T> {
    static constexpr uint32_t flagBit = (uint32_t)1 << 31;
    uint32_t data = 0;
    Ref() { }
    Ref(const MakeSelfReference& selfRef) { set(selfRef(this)); }
    Ref(T value) { set(value); }
    bool isRef() const { return data & flagBit; }
    bool isValue() const { return !isRef(); }
    void set(T value) { data = std::bit_cast<uint8_t>(value); }
    void set(RefInfo info) { data = info.compact() | flagBit; }
    T value() const { return std::bit_cast<T>((uint8_t)data); }
    RefInfo ref() const { return RefInfo::fromCompact(data & ~flagBit); }
    T resolve(const Resolver&) const;
};

/*{
    Ref<T> current = *this;
    while (current.isRef())
        current = *(Ref<T>*)style.fetch(ref());
    return current.value();
}*/

}