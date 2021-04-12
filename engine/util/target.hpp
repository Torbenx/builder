#pragma once

#ifdef _WIN32
#define PLATFORM_WINDOWS 1
#endif

#ifdef __ANDROID__
#define PLATFORM_ANDROID 1
#endif

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
