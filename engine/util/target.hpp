#pragma once

#include "util/types.hpp"

// PLATFORM

#ifdef _WIN32
#define PLATFORM_WINDOWS 1
#endif

#ifdef __ANDROID__
#define PLATFORM_ANDROID 1
#endif

// ARCHITECTURE

#if defined(__x86_64__) || defined(_M_X64)
#define ARCHITECTURE_X86_64
#endif

#if defined(__aarch64__) || defined(_M_ARM64)
#define ARCHITECTURE_ARM_64
#endif

// COMPILER STUFF

#if defined(__GNUC__)

#define ALIGN_AS(al) __attribute__((aligned(al)))

#define DEFINE_SECTION(type, name)                 \
    extern type __start_##name;                    \
    extern type __stop_##name;                     \
    inline type* __begin_##name = &__start_##name; \
    inline type* __end_##name = &__stop_##name;

#define IN_SECTION(name) __attribute__((section(#name)))

#elif defined(_MSC_VER)

#define ALIGN_AS(al) __declspec(align(al))

#define SECTION_POSTFIX_HELPER(s) #s
#define SECTION_POSTFIX(name, postfix) SECTION_POSTFIX_HELPER(name##$##postfix)

#define IN_SECTION_P(name, postfix) __declspec(allocate(SECTION_POSTFIX(name, postfix)))
#define IN_SECTION(name) IN_SECTION_P(name, m)

// clang-format off
#define DEFINE_SECTION(type, name)                     \
    __pragma(section(SECTION_POSTFIX(name, a)))       \
    __pragma(section(SECTION_POSTFIX(name, m)))       \
    __pragma(section(SECTION_POSTFIX(name, z)))       \
    inline type IN_SECTION_P(name, a) __start_##name;  \
    inline type IN_SECTION_P(name, z) __stop_##name;   \
    inline type* __begin_##name = &__start_##name + 1; \
    inline type* __end_##name = &__stop_##name;
// clang-format on

#else

#error Unsupported compiler

#endif

#define DEFINE_LINK_COUNTER(name) DEFINE_SECTION(char, name)
#define LINK_COUNTER_IMPL(name)                                   \
    static inline char IN_SECTION(name) __link_counter_dummy = 0; \
    static inline int_t link_counter() { return &__link_counter_dummy - &__start_##name; }
#define LINK_COUNTER_MAX(name) (&__stop_##name - &__start_##name)

namespace platform {

namespace detail {
    extern size_t pageSize;
}

static consteval size_t cachelineSize() { return 64; }
inline size_t pageSize() { return detail::pageSize; }
inline bool isPageAligned(uintptr_t addr) { return (addr & (pageSize() - 1)) == 0; }
inline bool isPageAligned(void* addr) { return isPageAligned((uintptr_t)addr); }

void* reserveAddressSpace(size_t size);
void commitMemory(void* addr, size_t size);

constexpr bool isWindows() {
#ifdef PLATFORM_WINDOWS
    return true;
#else
    return false;
#endif
}

constexpr bool isAndroid() {
#ifdef PLATFORM_ANDROID
    return true;
#else
    return false;
#endif
}

constexpr bool rawInput() {
    return isWindows();
}

constexpr bool usingGLES() {
    return isAndroid();
}

}
