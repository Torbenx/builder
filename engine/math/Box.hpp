#pragma once

#include "util/log.hpp"
#include "math/Vec3.hpp"
#include <math.h>

class Box {
public:
    static const Box INFINIT;

    float left, right, bottom, top, front, back;

    void enlargeToInclude(Vec3 p) {
        left = fmin(left, p.x);
        right = fmax(right, p.x);
        bottom = fmin(bottom, p.y);
        top = fmax(top, p.y);
        front = fmin(front, p.z);
        back = fmax(back, p.z);
    }

    void debug() {
        log<Tag::Math>("left={}, bottom={}, front={}, right={}, top={}, back={}", left, bottom, front, right, top, back);
    }
};
