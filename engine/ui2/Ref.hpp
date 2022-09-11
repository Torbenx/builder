#pragma once

#include "util/types.hpp"
#include <bit>

namespace ui {

struct Resolver;

struct RefInfo {
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
template<typename T>
struct Ref<T*> : RefPtrBase {
    Ref() { }
    Ref(const MakeSelfReference& selfRef) { set(selfRef(this)); }
    T* value() const { return (T*)RefPtrBase::value(); }
    T* resolve(const Resolver&); /*const {
        Ref<T*> current = *this;
        while (current.isRef())
            current = *(Ref<T*>*)style.fetch(ref());
        return current.value();
    }*/
};
template<typename T> requires (sizeof(T) <= sizeof(uint32_t))
struct Ref<T> {
    uint32_t lowPart = 0;
    uint32_t highPart = 0;
    using storage_t = std::conditional_t<sizeof(T) == 1, uint8_t, std::conditional_t<sizeof(T) == 2, uint16_t, uint32_t>>;
    Ref() { }
    Ref(const MakeSelfReference& selfRef) { set(selfRef(this)); }
    Ref(const T& value)
        : lowPart(0), highPart(std::bit_cast<storage_t>(value)) { }
    bool isRef() const { return lowPart & 1; }
    bool isValue() const { return !isRef(); }
    T value() const { return std::bit_cast<T>((storage_t)highPart); }

    RefInfo ref() const { return { .styleKey = highPart, .byteOffset = lowPart >> 2 }; }
    void set(RefInfo info) {
        highPart = info.styleKey;
        lowPart = (info.byteOffset << 2) | (uint32_t)1;
    }
    T resolve(const Resolver&) const;/* {
        Ref<T> current = *this;
        while (current.isRef())
            current = *(Ref<T>*)style.fetch(ref());
        return current.value();
    }*/
};

}