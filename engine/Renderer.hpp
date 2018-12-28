#ifndef HEADER_RENDERER_H
#define HEADER_RENDERER_H

#include "input/Pointer.hpp"

class Renderer{
    public:
        virtual void create(){}
        virtual void change(int w,int h){}
        virtual void drawFrame(){}
        virtual void key(){}
        virtual void pointer(PointerEvent event){}
        virtual void destroy(){}
        virtual ~Renderer(){}
};

#endif
