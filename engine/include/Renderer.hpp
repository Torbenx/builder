#ifndef HEADER_RENDERER_H
#define HEADER_RENDERER_H

class Renderer{
	public:
		virtual void create() = 0;
		virtual void change(int w,int h) = 0;
		virtual void drawFrame() = 0;
	
};

#endif
