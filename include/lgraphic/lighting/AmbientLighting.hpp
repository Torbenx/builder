#ifndef HEADER_LGAPHIC_LIGHTING_DIFFUSELIGHTING_H
#define HEADER_LGAPHIC_LIGHTING_DIFFUSELIGHTING_H

#include "../../math/Vec4.hpp"

class AmbientLighting{
	public:
		Vec3 color;
		AmbientLighting(Vec3 color){
			this->color = color;
		}
};

#endif
