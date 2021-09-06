#pragma once

#include "math/Vec4.hpp"

namespace std140 {

using vec2 __attribute__((aligned(8))) = Vec4;
using vec3 __attribute__((aligned(16))) = Vec3;
using vec4 __attribute__((aligned(16))) = Vec4;
using mat4 __attribute__((aligned(16))) = Mat4x4;

template<typename T>
concept Struct = std::is_standard_layout_v<T>;

template<Struct T, size_t N>
using array = std::array<T, N>;

}
