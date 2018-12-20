#ifndef HEADER_UTIL_RENDERABLE_H
#define HEADER_UTIL_RENDERABLE_H

#include <string>
#include "../math/MvpMatrix.hpp"
#include "../graphic/Shader.hpp"

class Renderable{
	public:
		virtual void draw(
			Shader *shader,
			MvpMatrix **mvpMatrices,
			std::string positionAttrib,
			std::string normalAttrib,
			std::string colorAttrib,
			std::string textureUniform,
			std::string colorUniform,
			std::string *mvpMatrixUniforms,
			int matrixCount) = 0;
};

#endif
