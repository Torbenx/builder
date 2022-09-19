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
    mutable const std::byte* baseAddress = nullptr;
    uint32_t styleKey = 0;
    MakeSelfReference(int_t styleKey)
        : styleKey(styleKey) { }
    RefInfo operator()(const void* thiz) const {
        if (!baseAddress)
            baseAddress = (const std::byte*)thiz;
        return { .styleKey = styleKey, .byteOffset = (uint32_t)((const std::byte*)thiz - baseAddress) };
    }
};

template<typename T>
struct Ref;

template<typename T>
struct RefLeaf;

struct RefPtrBase {
    uintptr_t data = 0;
    bool isRef() const { return data & 1; }
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
struct RefLeaf<T*> : RefPtrBase {
    T* value() const { return (T*)RefPtrBase::value(); }
};

template<>
struct RefLeaf<Size> {
    Size data;
    static constexpr uint32_t payloadMask = Size::payloadUintMask;
    static_assert(payloadMask == (uint32_t)1 << 30);
    bool isRef() const { return data.payloadBit(); }
    void set(Size size) {
        VERIFY(Ui, !size.payloadBit());
        data = size;
    }
    Size value() const {
        return data.withoutPayload();
    }
    void set(RefInfo info) {
        data = Size::fromUint(info.compact() | payloadMask);
    }
    RefInfo ref() const {
        return RefInfo::fromCompact(data.uint() & ~payloadMask);
    }
};

template<>
struct RefLeaf<float> {
    static constexpr uint32_t flagBit = (uint32_t)1 << TagFloat<1>::tagPos;
    static_assert(flagBit == (uint32_t)1 << 30);
    TagFloat<1> data;
    bool isRef() const { return data.tag(); }
    void set(float value) { data = { value, 0 }; }
    void set(RefInfo info) { data = TagFloat<1>::fromUint(info.compact() | flagBit); }
    float value() const { return data.value(); }
    RefInfo ref() const { return RefInfo::fromCompact(data.uint() & ~flagBit); }
};

template<typename T>
struct SmallRefStorage32;
template<typename T>
requires(sizeof(T) == 1) struct SmallRefStorage32<T> {
    static uint32_t pack(T value) { return std::bit_cast<uint8_t>(value); };
    static T unpack(uint32_t storage) { return std::bit_cast<T>((uint8_t)storage); }
};
template<typename T>
requires(sizeof(T) == 2) struct SmallRefStorage32<T> {
    static uint32_t pack(T value) { return std::bit_cast<uint16_t>(value); };
    static T unpack(uint32_t storage) { return std::bit_cast<T>((uint16_t)storage); }
};
template<typename T>
requires(sizeof(T) == 3) struct SmallRefStorage32<T> {
    struct Storage {
        T value;
        char padding = 0;
    };
    static uint32_t pack(T value) { return std::bit_cast<uint32_t>(Storage { value }); };
    static T unpack(uint32_t storage) { return std::bit_cast<Storage>(storage).value; }
};
template<typename T>
requires(sizeof(T) == 4 && (std::is_integral_v<T> || std::is_enum_v<T>)) struct SmallRefStorage32<T> {
    struct Storage {
        T value : 31;
    };
    static uint32_t pack(T value) { return std::bit_cast<uint32_t>(Storage { value }); }
    static T unpack(uint32_t storage) { return std::bit_cast<Storage>(storage).value; }
};

template<typename T>
requires requires { typename SmallRefStorage32<T>; }
struct RefLeaf<T> {
    static constexpr uint32_t flagBit = (uint32_t)1 << 31;
    uint32_t data = 0;
    bool isRef() const { return data & flagBit; }
    void set(T value) { data = SmallRefStorage32<T>::pack(value) & ~flagBit; }
    void set(RefInfo info) { data = info.compact() | flagBit; }
    T value() const { return SmallRefStorage32<T>::unpack(data); }
    RefInfo ref() const { return RefInfo::fromCompact(data & ~flagBit); }
};

template<typename T>
requires requires { typename RefLeaf<T>; }
struct Ref<T> : RefLeaf<T> {
    Ref() { }
    Ref(const MakeSelfReference& selfRef) { this->set(selfRef(this)); }
    Ref(T value) { this->set(value); }
    bool isValue() const { return !this->isRef(); }
    T resolve(const Resolver&) const;
};

/*{
    Ref<T> current = *this;
    while (current.isRef())
        current = *(Ref<T>*)style.fetch(ref());
    return current.value();
}*/

}