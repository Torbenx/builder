#pragma once

#include "FixedPoint.hpp"
#include "Size.hpp"
#include "util/types.hpp"
#include <bit>

namespace ui {

struct Resolver;
struct Substituter;

template<typename T>
struct alignas(4) wrapper {
    T value;
};

struct PtrInfo {
    static constexpr int COMPACT_OFFSET_BITS = 12;
    static constexpr uint32_t MIN_OFFSET_ALIGNMENT = 4;
    static constexpr uint32_t MAX_OFFSET = ((size_t)1 << COMPACT_OFFSET_BITS) * MIN_OFFSET_ALIGNMENT;
    static constexpr int COMPACT_KEY_BITS = 30 - COMPACT_OFFSET_BITS;
    static constexpr uint32_t MAX_KEY = (size_t)1 << COMPACT_KEY_BITS;
    static PtrInfo fromCompact(uint32_t comp) {
        return {
            .styleKey = comp >> COMPACT_OFFSET_BITS,
            .byteOffset = (comp & (((uint32_t)1 << COMPACT_OFFSET_BITS) - 1)) * MIN_OFFSET_ALIGNMENT
        };
    }
    // generate compact 30-bit representation
    uint32_t compact() const {
        VERIFY(Ui, styleKey < MAX_KEY);
        VERIFY(Ui, stx::isAligned(MIN_OFFSET_ALIGNMENT, byteOffset));
        VERIFY(Ui, byteOffset < MAX_OFFSET);
        return (styleKey << COMPACT_OFFSET_BITS) | (byteOffset / MIN_OFFSET_ALIGNMENT);
    }
    uint32_t styleKey = 0;
    uint32_t byteOffset = 0;

    bool isNull() const { return styleKey == 0; }
    PtrInfo offsetBy(int_t offset) const {
        return { styleKey, (uint32_t)(byteOffset + offset) };
    }
};

template<typename T = void>
struct Ptr;

struct PtrBase {
    uint32_t storage = 0;
    PtrBase() = default;
    explicit PtrBase(uint32_t compactInfo)
        : storage(compactInfo) { }
    PtrBase(PtrInfo info)
        : PtrBase(info.compact()) { }
    explicit PtrBase(Ptr<void>);
    PtrInfo info() const { return PtrInfo::fromCompact(storage); }
    operator Ptr<void>() const;
    Ptr<void> byteOffsetBy(uint32_t) const;
    explicit operator bool() const { return storage != 0; }
};
template<typename T>
struct Ptr : PtrBase {
    using PtrBase::PtrBase;
};
inline PtrBase::PtrBase(Ptr<void> p)
    : storage(p.storage) { }
inline PtrBase::operator Ptr<void>() const { return Ptr<void> { storage }; }
inline Ptr<void> PtrBase::byteOffsetBy(uint32_t offset) const {
    return Ptr<void> { storage + offset / PtrInfo::MIN_OFFSET_ALIGNMENT };
}

// recursive ptr or value
template<typename T>
struct Ref;

template<typename T>
struct RefLeaf;

struct RefPointerLeaf {
    uintptr_t data = 0;
    bool isValue() const { return data & 1; }
    void set(void* value) {
        data = (uintptr_t)value | (uintptr_t)1;
    }
    void* value() const {
        return (void*)(data & ~(uintptr_t)1);
    }
    void set(Ptr<void> info) {
        data = (uintptr_t)info.storage << 32;
    }
    Ptr<void> ptr() const {
        return Ptr<void> { (uint32_t)(data >> 32) };
    }
};
template<typename T>
struct RefLeaf<T*> : RefPointerLeaf {
    T* value() const { return (T*)RefPointerLeaf::value(); }
    Ptr<T> ptr() const { return (Ptr<T>)RefPointerLeaf::ptr(); }
};

template<>
struct RefLeaf<Size> {
    Size data;
    static constexpr uint32_t payloadMask = Size::payloadUintMask;
    static_assert(payloadMask == (uint32_t)1 << 30);
    bool isValue() const { return data.payloadBit(); }
    void set(Size size) {
        VERIFY(Ui, !size.payloadBit());
        data = size.withPayload();
    }
    Size value() const {
        return data.withoutPayload();
    }
    void set(Ptr<Size> ptr) {
        data = Size::fromUint(ptr.storage);
    }
    Ptr<Size> ptr() const {
        return Ptr<Size>(data.uint());
    }
};

template<>
struct RefLeaf<float> {
    static constexpr uint32_t flagBit = (uint32_t)1 << TagFloat<1>::tagPos;
    static_assert(flagBit == (uint32_t)1 << 30);
    TagFloat<1> data;
    bool isValue() const { return data.tag(); }
    void set(float value) { data = { value, 1 }; }
    void set(Ptr<float> ptr) { data = TagFloat<1>::fromUint(ptr.storage); }
    float value() const { return data.value(); }
    Ptr<float> ptr() const { return Ptr<float>(data.uint()); }
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
template<>
struct SmallRefStorage32<Color> {
    static uint32_t pack(Color value) {
        uint32_t v = value.uint();
        return (v & 0x00'ff'ff'ff) | ((v & 0xfe'00'00'00) >> 1);
    }
    static Color unpack(uint32_t s) {
        return (s + (s & 0x7f'00'00'00)) | ((s & 0x40'00'00'00) >> 6);
    }
};

template<typename T>
requires requires { typename SmallRefStorage32<T>; }
struct RefLeaf<T> {
    static constexpr uint32_t flagBit = (uint32_t)1 << 31;
    uint32_t data = 0;
    bool isValue() const { return data & flagBit; }
    void set(T value) { data = SmallRefStorage32<T>::pack(value) | flagBit; }
    void set(Ptr<T> ptr) { data = ptr.storage; }
    T value() const { return SmallRefStorage32<T>::unpack(data & ~flagBit); }
    Ptr<T> ptr() const { return Ptr<T>(data); }
};

template<typename T>
T resolveLeaf(const Ref<T>& in, const Resolver& Resolver);
template<typename T>
T substituteLeaf(const Ref<T>& in, const Substituter& substituter);

template<typename T>
requires requires { typename RefLeaf<T>; }
struct Ref<T> : RefLeaf<T> {
    Ref() { }
    Ref(Ptr<T> ptr) { this->set(ptr); }
    Ref(T value) { this->set(value); }
    bool isPtr() const { return !this->isValue(); }
    void clear() { this->set(Ptr<T>()); }
    T resolve(const Resolver& resolver) const { return resolveLeaf<T>(*this, resolver); }
    T substitute(const Substituter& substituter, const T& base) const { return substituteLeaf<T>(*this, substituter, base); }
};

}