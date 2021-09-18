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

// ALIGNMENT

#if defined(__GNUC__)
#define ALIGN_AS(al) __attribute__((aligned(al)))
#elif defined(_MSC_VER)
#define ALIGN_AS(al) __declspec(align(al))
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
