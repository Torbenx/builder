#ifndef HEADER_INPUT_LOOK3D_H
#define HEADER_INPUT_LOOK3D_H

#include "Pointer.hpp"
#include "../math/Mat4x4.hpp"

class Look3d{
    private:
        Pointer lpointer;
    public:
        Vec3 dir,up;
        bool invert;
        Look3d(Vec3 up,bool invert);
        void move(float dx, float dy);
};

#endif