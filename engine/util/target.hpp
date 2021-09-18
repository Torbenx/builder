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

#ifdef __x86_64__
#define ARCHITECTURE_X86_64
#endif

#ifdef __aarch64__
#define ARCHITECTURE_ARM_64
#endif

// COMPILER STUFF

#if defined(__GNUC__)

#define ALIGN_AS(al) __attribute__((aligned(al)))

#define DEFINE_SECTION(name)

#define IN_SECTION(name) __attribute__((section(#name)))

#define DECLARE_SECTION_START_STOP(type, name) \
    extern type __start_##name;                \
    extern type __stop_##name;

#elif defined(_MSC_VER)

#define ALIGN_AS(al) __declspec(align(al))

#define SECTION_POSTFIX_HELPER(s) #s
#define SECTION_POSTFIX(name, postfix) SECTION_POSTFIX_HELPER(name##$##postfix)

#define DEFINE_SECTION(name)                    \
    __pragma(data_seg(SECTION_POSTFIX(name, a))) \
    __pragma(data_seg(SECTION_POSTFIX(name, m))) \
    __pragma(data_seg(SECTION_POSTFIX(name, z)))

#define IN_SECTION_P(name, postfix) __declspec(allocate(SECTION_POSTFIX(name, postfix)))
#define IN_SECTION(name) IN_SECTION_P(name, m)

#define DECLARE_SECTION_START_STOP(type, name) \
    type IN_SECTION_P(name, a) __start_##name; \
    type IN_SECTION_P(name, z) __stop_##name;

#else

#error Unsupported compiler

#endif

namespace platform {

namespace detail {
    extern size_t pageSize;
}

static consteval size_t cachelineSize() { return 64; }
inline size_t pageSize() { return detail::pageSize; }

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
