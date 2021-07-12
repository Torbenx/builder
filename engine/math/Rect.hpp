#pragma once

#include "math/Vec2.hpp"
#include <array>
#include <stddef.h>

class Rect {
public:
    float top, bottom, left, right;
    float width();
    float height();
    Vec2 topLeft();
    Vec2 topRight();
    Vec2 bottomRight();
    Vec2 bottomLeft();
    std::array<Vec2, 4> corners();
    bool contains(Vec2 p);
    void debug();
    Rect operator+=(Vec2 v);
    Rect operator-=(Vec2 v);
    Rect operator=(Rect other);
    static Rect btlr(float bottom, float top, float left, float right);
    static Rect bltr(float bottom, float left, float top, float right);
    static Rect tlbr(float top, float left, float bottom, float right);
};
