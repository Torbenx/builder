#pragma once

#include "util/target.hpp"

namespace plt {

inline constexpr uint32_t TLS_MAIN_TICK_INDEX = 0;
inline constexpr uint32_t TLS_SCHEDULER_INDEX = 1;
inline constexpr uint32_t TLS_TIMELINE_INDEX = 2;
inline constexpr uint32_t TLS_GRAPHICS_BUFFER_MANAGER_INDEX = 3;
inline constexpr int_t TLS_INDICES_COUNT = 4;

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

inline constexpr uint32_t TLS_INDICES_BASE = 20;
template<uint32_t index>
uintptr_t getThreadLocalValue() {
    // return NtCurrentTeb()->TlsSlots[TLS_INDICES_BASE + index];
    return read_segment_register(__builtin_offsetof(_TEB, TlsSlots[TLS_INDICES_BASE + index]));
}
template<uint32_t index>
void setThreadLocalValue(uintptr_t value) {
    write_segment_register(__builtin_offsetof(_TEB, TlsSlots[TLS_INDICES_BASE + index]), value);
}

}

#else

namespace plt {

namespace detail {
    constinit inline thread_local uintptr_t thread_local_storage alignas(platform::cachelineSize())[TLS_INDICES_COUNT] = {};
}
template<uint32_t index>
uintptr_t getThreadLocalValue() {
    return thread_local_storage[index];
}
template<uint32_t index>
void setThreadLocalValue(uintptr_t value) {
    thread_local_storage[index] = value;
}

}

#endif

#define DECLARE_THREAD_LOCAL_GETSET(type, getName, setName, indexName)                               \
    inline type getName() { return (type)plt::getThreadLocalValue<plt::TLS_##indexName##_INDEX>(); } \
    inline void setName(type value) { plt::setThreadLocalValue<plt::TLS_##indexName##_INDEX>((uintptr_t)value); }
