#pragma once

#include "math/Vec4.hpp"

namespace std140 {

typedef ALIGN_AS(8) Vec2 vec2;
typedef ALIGN_AS(16) Vec3 vec3;
typedef ALIGN_AS(16) Vec4 vec4;
typedef ALIGN_AS(16) Mat4x4 mat4;

template<typename T>
concept Struct = std::is_standard_layout_v<T>;

template<Struct T, size_t N>
using array = std::array<T, N>;

}
