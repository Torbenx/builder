#pragma once

#include <cstddef>
#include <cstdint>

typedef std::ptrdiff_t int_t;
typedef std::uint32_t id_t;
typedef std::uint16_t sid_t;

constexpr size_t alignmentCeil(size_t alignment, size_t in) { return (in + alignment - 1) & ~(alignment - 1); }
constexpr bool isAligned(size_t alignment, size_t in) { return (in & (alignment - 1)) == 0; }
