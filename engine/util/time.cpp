#include "time.hpp"


#ifdef PLATFORM_ANDROID

#include <time.h>
#include "log.hpp"

uint64_t getNanos(){
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * NANOS_PER_SECOND + (uint64_t)ts.tv_nsec;
}

uint64_t getEpochSeconds(){
    return time(nullptr);
}

#endif //PLATFORM_ANDROID


#ifdef PLATFORM_WINDOWS

#include <Windows.h>

uint64_t performenceFrequency;
#define EPOCH_TICKS 0x019DB1DED53E8000

void time_init(){
    LARGE_INTEGER t;
    QueryPerformanceFrequency(&t);
    performenceFrequency = t.QuadPart;
}

uint64_t getNanos(){
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    return t.QuadPart * NANOS_PER_SECOND / performenceFrequency;
}

uint64_t getEpochSeconds(){
    FILETIME t;
    GetSystemTimeAsFileTime(&t);
    return (((uint64_t)t.dwHighDateTime << 32) + (uint64_t)t.dwLowDateTime - EPOCH_TICKS) * 100 / NANOS_PER_SECOND;
}

#endif //PLATFORM_WINDOWS