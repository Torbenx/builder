#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>


namespace stx {

using int_t = std::ptrdiff_t;

constexpr size_t alignmentCeil(size_t alignment, size_t in) { return (in + alignment - 1) & ~(alignment - 1); }
constexpr bool isAligned(size_t alignment, size_t in) { return (in & (alignment - 1)) == 0; }

template<int_t... I>
using int_sequence = std::integer_sequence<int_t, I...>;
template<int_t N>
using make_int_sequence = std::make_integer_sequence<int_t, N>;
template<typename... Types>
using int_sequence_for = make_int_sequence<sizeof...(Types)>;

}

using int_t = stx::int_t;
using id_t = std::uint32_t;
using sid_t = std::uint16_t;

#ifdef _LIBCPP_VERSION

namespace std {
template<typename T, typename... Args>
constexpr auto construct_at(T* p, Args&&... args) {
    return new ((void*)p) T(std::forward<Args>(args)...);
}
}
#endif
