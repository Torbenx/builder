#include "util/log.hpp"
#include "windows/engine.hpp"
#include <iostream>
#include <string>

size_t platform::detail::pageSize;

int_t RendererState::getState() {
    return rendererState;
}

void RendererState::exit() {
    DestroyWindow(engine_hwnd);
}

void log_print(const std::string& tag, const std::string& msg) {
    std::string str = '[' + tag + "] " + msg + '\n';
    if constexpr (platform::hasMSVCCompilerInterface()) {
        OutputDebugStringA(str.c_str());
    } else {
        std::cout << str;
    }
}

void* platform::reserveMemoryRegion(size_t size) {
    VERIFY(Windows, isPageAligned(size));

    SYSTEM_INFO info;
    GetSystemInfo(&info);

    void* addr = VirtualAlloc(nullptr, size, MEM_RESERVE, PAGE_READWRITE);

    VERIFY(Windows, addr != nullptr);

    return addr;
}

void platform::commitMemory(void* addr, size_t size) {
    VERIFY(Windows, isPageAligned(addr));
    VERIFY(Windows, isPageAligned(size));
    void* ret = VirtualAlloc(addr, size, MEM_COMMIT, PAGE_READWRITE);
    VERIFY(Windows, ret == addr);
}

void platform::decommitMemory(void* addr, size_t size) {
    VERIFY(Windows, isPageAligned(addr));
    VERIFY(Windows, isPageAligned(size));
    bool ret = VirtualFree(addr, size, MEM_DECOMMIT);
    VERIFY(Windows, ret);
}

void platform::releaseMemoryRegion(void* addr, size_t) {
    VERIFY(Windows, isPageAligned(addr));
    bool ret = VirtualFree(addr, 0, MEM_RELEASE);
    VERIFY(Windows, ret);
}

void init_platform() {
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    platform::detail::pageSize = info.dwPageSize;
}
