#ifndef HEADER_UTIL_TIME_H
#define HEADER_UTIL_TIME_H

#include <stdint.h>

#define NANOS_PER_SECOND 1000000000

uint64_t getNanos();
uint64_t getEpochSeconds();

#endif