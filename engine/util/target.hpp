#pragma once

#include "util/types.hpp"

#ifdef _WIN32
#define PLATFORM_WINDOWS 1
#endif

#ifdef __ANDROID__
#define PLATFORM_ANDROID 1
#endif

#ifdef __x86_64__
#define ARCHITECTURE_X86_64
#endif

#ifdef __aarch64__
#define ARCHITECTURE_ARM_64
#endif

namespace platform {

static constexpr int_t CACHELINE_SIZE = 64;

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
