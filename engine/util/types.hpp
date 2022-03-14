#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>

typedef std::ptrdiff_t int_t;
typedef std::uint32_t id_t;
typedef std::uint16_t sid_t;

constexpr size_t alignmentCeil(size_t alignment, size_t in) { return (in + alignment - 1) & ~(alignment - 1); }
constexpr bool isAligned(size_t alignment, size_t in) { return (in & (alignment - 1)) == 0; }

namespace std {
template<typename T, typename... Args>
constexpr auto construct_at(T* p, Args&&... args) {
    return new ((void*)p) T(std::forward<Args>(args)...);
}
}
