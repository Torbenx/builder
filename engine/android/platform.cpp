#include "util/target.hpp"
#include "android/engine.hpp"
#include <sys/mman.h>
#include <unistd.h>
#include <android/log.h>

size_t platform::detail::pageSize;

void* platform::reserveAddressSpace(size_t size) {
    VERIFY(Android, isPageAligned(size));
    void* addr = mmap(nullptr, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    VERIFY(Android, addr != nullptr);
    return addr;
}

void platform::commitMemory(void* addr, size_t size) {
    VERIFY(Android, isPageAligned(addr));
    VERIFY(Android, isPageAligned(size));
    int ret = mprotect(addr, size, PROT_READ | PROT_WRITE);
    VERIFY(Android, ret == 0);
}

void init_platform() {
    platform::detail::pageSize = sysconf(_SC_PAGESIZE);
}

int_t RendererState::getState() {
    return rendererState;
}

void log_print(const std::string& tag, const std::string& msg) {
    __android_log_write(ANDROID_LOG_DEBUG, tag.c_str(), msg.c_str());
}