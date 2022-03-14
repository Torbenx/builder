#include "util/target.hpp"
#include "android/engine.hpp"
#include <sys/mman.h>
#include <unistd.h>
#include <android/log.h>

size_t platform::detail::pageSize;

void* platform::reserveMemoryRegion(size_t size, size_t alignment, void* baseAddress) {
    VERIFY(Android, isPageAligned(alignment));
    VERIFY(Android, isPageAligned(size));
    void* addr = mmap(nullptr, size + alignment, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    VERIFY(Android, addr != nullptr);

    if (alignment == 0)
        return addr;

    uintptr_t uAddr = (uintptr_t)addr;
    uintptr_t uAligned = alignmentCeil(alignment, uAddr);
    if (uAligned > uAddr) {
        int ret = munmap((void*)uAddr, uAligned - uAddr);
        VERIFY(Android, ret == 0);
    }
    if (uAligned + size < uAddr + size + alignment) {
        int ret = munmap((void*)(uAligned + size), alignment - (uAligned - uAddr));
        VERIFY(Android, ret == 0);
    }

    return (void*)uAligned;
}

void platform::commitMemory(void* addr, size_t size) {
    VERIFY(Android, isPageAligned(addr));
    VERIFY(Android, isPageAligned(size));
    int ret = mprotect(addr, size, PROT_READ | PROT_WRITE);
    VERIFY(Android, ret == 0);
}

void platform::decommitMemory(void* addr, size_t size) {
    VERIFY(Android, isPageAligned(addr));
    VERIFY(Android, isPageAligned(size));
    int ret = mprotect(addr, size, PROT_NONE);
    VERIFY(Android, ret == 0);
    ret = madvise(addr, size, MADV_DONTNEED);
    VERIFY(Android, ret == 0);
}

void platform::releaseMemoryRegion(void* addr, size_t size) {
    VERIFY(Android, isPageAligned(addr));
    VERIFY(Android, isPageAligned(size));
    int ret = munmap(addr, size);
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
