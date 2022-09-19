#pragma once

#include "util/target.hpp"

namespace plt {

constexpr uint32_t TLS_UI_INSTANCE_INDEX = 0;
constexpr int_t TLS_INDICES_COUNT = 1;

}

#ifdef PLATFORM_WINDOWS

#include "platform/windows/core.hpp"
#include <winternl.h>

namespace plt {

constexpr uint32_t TLS_INDICES_BASE = 20;
template<uint32_t index>
void* getThreadLocalPtr() {
    //return NtCurrentTeb()->TlsSlots[TLS_INDICES_BASE + index];
    return (void*)__readgsqword(__builtin_offsetof(_TEB, TlsSlots[TLS_INDICES_BASE + index]));
}
template<uint32_t index>
void setThreadLocalPtr(void* ptr) {
    __writegsqword(__builtin_offsetof(_TEB, TlsSlots[TLS_INDICES_BASE + index]), (uintptr_t)ptr);
}

}

#endif