#ifndef HEADER_RENDERER_H
#define HEADER_RENDERER_H

#include "input/Pointer.hpp"
#include <stdint.h>

/*
  method        | old state     | new state
  --------------|---------------|---------------
  create()      |               | invisible
  --------------|---------------|---------------
  visible()     | invisible     | background
  --------------|---------------|---------------
  foreground()  | background    | foreground
  --------------|---------------|---------------
  background()  | foreground    | background
  --------------|---------------|---------------
  invisible()   | background    | invisible
  --------------|---------------|---------------
  destroy()     | invisible     |
*/

#define RENDERER_STATE_INVISIBLE 1
#define RENDERER_STATE_BACKGROUND 2
#define RENDERER_STATE_FOREGROUND 3

class Renderer{
    public:
        static bool isInvisible();
        static bool isBackground();
        static bool isForeground();
        static int getState();
        static void exit();
        
        virtual void create(){}
        virtual void visible(){}
        virtual void background(){}
        virtual void foreground(){}
        virtual void invisible(){}
        virtual void destroy(){}
        
        virtual void change(int w,int h){}
        virtual void drawFrame(){}
        virtual void pointer(PointerEvent event){}
        virtual void rawPointer(int dx, int dy){}
        virtual void key(uint32_t keycode, bool down){}
        virtual void mouseKey(uint32_t key, bool down){}
        virtual ~Renderer(){}
};

#endif
