#ifndef HEADER_RENDERER_H
#define HEADER_RENDERER_H

#include "input/Pointer.hpp"
#include <stdint.h>

/*
  method        | old state     | new state
  --------------|---------------|---------------
  constructor   |               | invisible
  --------------|---------------|---------------
  visible()     | invisible     | background
  --------------|---------------|---------------
  foreground()  | background    | foreground
  --------------|---------------|---------------
  background()  | foreground    | background
  --------------|---------------|---------------
  invisible()   | background    | invisible
  --------------|---------------|---------------
  destructor    | invisible     |
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

        virtual void visible(){}
        virtual void background(){}
        virtual void foreground(){}
        virtual void invisible(){}

        virtual void change(int w,int h){}
        virtual void drawFrame(){}

        virtual void pointer(PointerEvent event){}

        virtual void rawPointer(int dx, int dy){}
        virtual void rawKey(uint32_t keycode, bool down){}
        virtual void rawMouseKey(uint32_t key, bool down){}
        //ticks > 0 => up
        virtual void rawMouseWheel(int ticks){}

        virtual ~Renderer(){}
};

#endif
