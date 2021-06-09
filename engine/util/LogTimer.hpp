#pragma once

#include "util/time.hpp"

class LogTimer {
public:
    uint64_t start;
    LogTimer() { start = getNanos(); }
    ~LogTimer() {
        uint64_t end = getNanos();
        LOGD("%llu us elapsed", (end - start) / 1000);
    }
};
