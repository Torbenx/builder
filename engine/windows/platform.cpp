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
    OutputDebugStringA(str.c_str());
    std::cout << str;
}

void* platform::reserveAddressSpace(size_t size) {
    VERIFY(Windows, size % pageSize() == 0);
    void* addr = VirtualAlloc(nullptr, size, MEM_RESERVE, PAGE_NOACCESS);
    VERIFY(Windows, addr != nullptr);
    return addr;
}

void platform::commitMemory(void* addr, size_t size) {
    VERIFY(Windows, (uintptr_t)addr % pageSize() == 0);
    VERIFY(Windows, size % pageSize() == 0);
    void* ret = VirtualAlloc(addr, size, MEM_COMMIT, PAGE_READWRITE);
    VERIFY(Windows, ret == addr);
}

void init_platform() {
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    platform::detail::pageSize = info.dwPageSize;
}