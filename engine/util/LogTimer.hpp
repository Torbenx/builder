#pragma once

#include "util/time.hpp"
#include <iostream>

class LogTimer {
public:
    uint64_t start;
    std::vector<uint64_t> laps;
    LogTimer() { start = getNanos(); }
    void operator()() { laps.push_back(getNanos()); }
    ~LogTimer() {
        (*this)();
        uint64_t prev = start;
        for (uint64_t t : laps) {
            std::cout << (t - prev) / 1000.0 << ", ";
            prev = t;
        }
        std::cout << "us\n";
    }
};
