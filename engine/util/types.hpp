#pragma once

#include <cstddef>
#include <cstdint>

typedef std::ptrdiff_t int_t;
typedef std::uint32_t id_t;
typedef std::uint16_t sid_t;

inline size_t alignmentCeil(size_t alignment, size_t in) { return (in + alignment - 1) & ~(alignment - 1); }
