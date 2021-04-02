#include "input/Look3d.hpp"
#include "util/log.hpp"

Look3d::Look3d(Vec3 up, bool invert){
    this->up = up.normalize();
    this->invert = invert;
}

void Look3d::move(float dxi, float dyi){
    float f = invert ? -1.0f : 1.0f;
    float dx = f*dxi / 1000.0f;
    float dy = -f*dyi / 1000.0f;
    Vec3 ex = Vec3::cross(dir,up);
    Vec3 ey = Vec3::cross(ex,dir).normalize();
    dir += dx * ex + dy * ey;
    dir.normalize();
}
