#pragma once

#include "math/Vec4.hpp"

namespace std140 {

struct alignas(8) vec2 : Vec2 {
    vec2()
        : Vec2() { }
    vec2(float x, float y)
        : Vec2(x, y) { }
    vec2(float xy)
        : Vec2(xy) { }
    vec2(Vec2 v)
        : Vec2(v) { }
};

struct alignas(16) vec3 : Vec3 {
    vec3()
        : Vec3() { }
    vec3(float x, float y, float z)
        : Vec3(x, y, z) { }
    vec3(float xyz)
        : Vec3(xyz) { }
    vec3(Vec3 v)
        : Vec3(v) { }
};

struct alignas(16) vec4 : Vec4 {
    vec4()
        : Vec4() { }
    vec4(float x, float y, float z, float w)
        : Vec4(x, y, z, w) { }
    vec4(Vec4 v)
        : Vec4(v) { }
};

template<typename T>
concept Struct = std::is_standard_layout_v<T>;

template<Struct T, size_t N>
using array = std::array<T, N>;

}
