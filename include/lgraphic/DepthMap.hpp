#ifndef HEADER_LGRAPHIC_DEPTHMAP_H
#define HEADER_LGRAPHIC_DEPTHMAP_H

#include "../graphic/Shader.hpp"
#include "../graphic/Framebuffer.hpp"
#include "../util/Renderable.hpp"
#include "../util/util.hpp"
#include <string.h>

class DepthMap{
	public:
		static const std::string POSITION_ATTRIB;
		static const std::string MVP_MATRIX_UNIFORM;
		Texture texture = Texture();
		Renderbuffer renderbuffer = Renderbuffer();
		Framebuffer framebuffer = Framebuffer();
		bool divideByW;
		int width,height;
		float m_far;
		Shader *shader;
		std::string fSource,vSource;
		DepthMap(bool divideByW,int width,int height,float far);
		~DepthMap();
		void init();
		void draw(MvpMatrix& mvpMatrix,Renderable **ptrArray,int count);
};

#endif
