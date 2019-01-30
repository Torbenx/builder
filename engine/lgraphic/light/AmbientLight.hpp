#ifndef HEADER_LGAPHIC_LIGHT_AMBIENTLIGHT_H
#define HEADER_LGAPHIC_LIGHT_AMBIENTLIGHT_H

#include "../../math/Vec4.hpp"

class AmbientLight{
    public:
        Vec3 color;
        AmbientLight(Vec3 color){
            this->color = color;
        }
};

#endif
