#include "Look3d.hpp"
#include "../util/log.hpp"

Look3d::Look3d(Vec3 up, bool invert){
    this->up = up.normalize();
    this->invert = invert;
    lpointer.pointerId = POINTER_ID_INVALID;
}

void Look3d::pointer(PointerEvent event){
    if(getPointerCount() == 1){
        Pointer pointer = getPointerByIndex(0);
        if(pointer.pointerId == lpointer.pointerId){
            float f = invert ? -1.0f : 1.0f;
            float dx = f*(pointer.x - lpointer.x) / 1000.0f;
            float dy = -f*(pointer.y - lpointer.y) / 1000.0f;
            Vec3 ex = cross(dir,up).normalize();
            Vec3 ey = cross(ex,dir);
            dir += dx * ex + dy * ey;
            dir.normalize();
        }
        lpointer = pointer;
    }else{
        lpointer.pointerId = POINTER_ID_INVALID;
    }
}

void Look3d::teleportPointer(PointerEvent event){
    if(event.pointerId == lpointer.pointerId){
        lpointer.x = event.x;
        lpointer.y = event.y;
    }
}