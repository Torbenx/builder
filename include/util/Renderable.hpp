#ifndef HEADER_UTIL_RENDERABLE_H
#define HEADER_UTIL_RENDERABLE_H

#include <string>
#include "../math/MvpMatrix.hpp"
#include "../graphic/Shader.hpp"

class Renderable{
	public:
		virtual void draw(
			Shader *shader,
			std::string positionAttrib,
			std::string modelUniform
		) = 0;
};

#endif
