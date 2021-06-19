#pragma once

#include "util/time.hpp"
#include <iostream>

class LogTimer {
public:
    uint64_t start;
    std::vector<uint64_t> laps;

    LogTimer() { start = getNanos(); }

    void operator()() {
        laps.push_back(getNanos());
    }

    void disarm() {
        start = 0;
    }

    ~LogTimer() {
        if (start == 0)
            return;
        (*this)();
        uint64_t prev = start;
        int i = 0;
        for (uint64_t t : laps) {
            // std::cout << (t - prev) / 1000.0 << ", ";
            LOGD("%u: %f us,", i++, (t - prev) / 1000.0);
            prev = t;
        }
        LOGD("----------------------");
        // std::cout << "us\n";
    }
};
