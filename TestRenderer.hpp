#ifndef HEADER_TESTRENDERER_H
#define HEADER_TESTRENDERER_H

#include "Renderer.hpp"

class TestRenderer : public Renderer{
	public:
		void create();
		void change(int width, int height);
		void drawFrame();
};

#endif