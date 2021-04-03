#pragma once

#include "Pointer.hpp"
#include "../math/Mat4x4.hpp"

class Look3d{
public:
    Look3d(Vec3 up, bool invert);
    void move(float dx, float dy);
    Vec3 dir, up;
    bool invert;
private:
    Pointer lpointer;
};
