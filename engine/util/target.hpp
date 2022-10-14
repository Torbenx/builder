#pragma once

#include "containers/ArrayView.hpp"
#include "util/log.hpp"
#include <bit>

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

namespace platform {

template<typename T>
inline ArrayView<T> normalizeSectionBounds(T* begin, T* end) {
    return { begin, end - begin };
}

}

#if defined(__GNUC__)

#define COMPILER_INTERFACE_GNU

#if defined(__clang__)
#define COMPILER_INTERNALS_CLANG
#else
#define COMPILER_INTERNALS_GCC
#endif

#define ASSUME(expr)                 \
    {                                \
        if (!(expr)) {               \
            __builtin_unreachable(); \
        }                            \
    }

#define ALIGN_AS(al) __attribute__((aligned(al)))

#define DEFINE_SECTION(type, name)                                                    \
    extern type __start_##name;                                                       \
    extern type __stop_##name;                                                        \
    namespace sections {                                                              \
        inline ArrayView<type> name() {                                               \
            return platform::normalizeSectionBounds(&__start_##name, &__stop_##name); \
        }                                                                             \
    }

#define IN_SECTION(name) __attribute__((section(#name)))

#define MAKE_WEAK_SYMBOL(name)
#define WEAK_SYMBOL_IMPL(name) __attribute__((weak)) name
#define WEAK_SYMBOL_DECL(name) __attribute__((weak)) name

#elif defined(_MSC_VER)

#define COMPILER_INTERFACE_MSVC

#if defined(__clang__)
#define COMPILER_INTERNALS_CLANG
#else
#define COMPILER_INTERNALS_MSVC
#endif

#define ASSUME(expr) __assume(expr)

#define ALIGN_AS(al) __declspec(align(al))

#define SECTION_POSTFIX_HELPER(s) #s
#define SECTION_POSTFIX(name, postfix) SECTION_POSTFIX_HELPER(name##$##postfix)

#define IN_SECTION_P(name, postfix) __declspec(allocate(SECTION_POSTFIX(name, postfix)))
#define IN_SECTION(name) IN_SECTION_P(name, m)

// clang-format off
#define DEFINE_SECTION(type, name)                                \
    __pragma(section(SECTION_POSTFIX(name, a)))                   \
    __pragma(section(SECTION_POSTFIX(name, m)))                   \
    __pragma(section(SECTION_POSTFIX(name, z)))                   \
    inline type IN_SECTION_P(name, a) __before_begin_##name = {}; \
    inline type IN_SECTION_P(name, z) __after_end_##name = {};    \
    namespace sections {                                          \
        inline ArrayView<type> name() {                           \
            return platform::normalizeSectionBounds(&__before_begin_##name + 1, &__after_end_##name); \
        }                                                         \
    }
// clang-format on

#define MAKE_WEAK_SYMBOL(name) __pragma(comment(linker, "/alternatename:" #name "=" #name "__weak"))
#define WEAK_SYMBOL_IMPL(name) name##__weak
#define WEAK_SYMBOL_DECL(name) name

#else

#error Unsupported compiler

#endif

#define SECTION_ELEMENT_COUNT(name) (__end_##name - __begin_##name)

#define DEFINE_LINK_COUNTER(name) DEFINE_SECTION(char, name)
#define LINK_COUNTER_IMPL(name)                                                   \
    static inline constinit const char IN_SECTION(name) __link_counter_dummy = 0; \
    static inline int_t linkCounter() { return &__link_counter_dummy - __begin_##name; }
#define LINK_COUNTER_MAX(name) SECTION_ELEMENT_COUNT(name)

namespace platform {

namespace detail {
    extern size_t pageSize;
}

consteval size_t cachelineSize() { return 64; }
inline size_t pageSize() { return detail::pageSize; }
inline int_t logPageSize() { return std::countr_zero(pageSize()); }
inline bool isPageAligned(uintptr_t addr) { return (addr & (pageSize() - 1)) == 0; }
inline bool isPageAligned(void* addr) { return isPageAligned((uintptr_t)addr); }

void* reserveMemoryRegion(size_t size);
void commitMemory(void* addr, size_t size);
void decommitMemory(void* addr, size_t size);
// can only release the entire region at once
void releaseMemoryRegion(void* addr, size_t size);

consteval bool isWindows() {
#ifdef PLATFORM_WINDOWS
    return true;
#else
    return false;
#endif
}
consteval bool isAndroid() {
#ifdef PLATFORM_ANDROID
    return true;
#else
    return false;
#endif
}

consteval bool hasGNUCompilerInterface() {
#ifdef COMPILER_INTERFACE_GNU
    return true;
#else
    return false;
#endif
}
consteval bool hasMSVCCompilerInterface() {
#ifdef COMPILER_INTERFACE_MSVC
    return true;
#else
    return false;
#endif
}

consteval bool rawInput() {
    return isWindows();
}

consteval bool usingGLES() {
    return isAndroid();
}

}
