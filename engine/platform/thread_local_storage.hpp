#pragma once

#include "util/target.hpp"

namespace plt {

constexpr uint32_t TLS_CURRENT_TICK_INDEX = 0;
constexpr uint32_t TLS_CURRENT_EXECUTOR_INDEX = 1;
constexpr uint32_t TLS_UI_INSTANCE_INDEX = 2;
constexpr int_t TLS_INDICES_COUNT = 3;

}

#ifdef PLATFORM_WINDOWS

#include "platform/windows/core.hpp"
#include <winternl.h>

namespace plt {

#if defined(COMPILER_INTERNALS_MSVC)
inline void write_segment_register(uint32_t offset, uint64_t data) {
    __writegsqword(offset, data);
}
inline uint64_t read_segment_register(uint32_t offset) {
    return __readgsqword(offset);
}
#else
// based mingw's implentation of read/writegsqword
static_assert(sizeof(void*) == 8);
inline void write_segment_register(uint32_t offset, uint64_t data) {
    asm("mov{q %[data], %%gs:%[offset] | %%gs:%[offset], %[data]}"
            : [offset] "=m"((*(uint64_t*)(uintptr_t)offset))
            : [data] "ri"(data));
}
inline uint64_t read_segment_register(uint32_t offset) {
    uint64_t ret;
    asm("mov{q %%gs:%[offset], %[ret] | %[ret], %%gs:%[offset]}"
            : [ret] "=r"(ret)
            : [offset] "m"((*(uint64_t*)(uintptr_t)offset)));
    return ret;
}
#endif

constexpr uint32_t TLS_INDICES_BASE = 20;
template<uint32_t index>
void* getThreadLocalPtr() {
    // return NtCurrentTeb()->TlsSlots[TLS_INDICES_BASE + index];
    return (void*)read_segment_register(__builtin_offsetof(_TEB, TlsSlots[TLS_INDICES_BASE + index]));
}
template<uint32_t index>
void setThreadLocalPtr(void* ptr) {
    write_segment_register(__builtin_offsetof(_TEB, TlsSlots[TLS_INDICES_BASE + index]), (uintptr_t)ptr);
}

}

#else

namespace plt {

namespace detail {
    constinit inline thread_local void* thread_local_storage alignas(platform::cachelineSize())[TLS_INDICES_COUNT] = {};
}
template<uint32_t index>
void* getThreadLocalPtr() {
    return thread_local_storage[index];
}
template<uint32_t index>
void setThreadLocalPtr(void* ptr) {
    thread_local_storage[index] = ptr;
}

}

#endif