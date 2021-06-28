#pragma once

#include "input/Pointer.hpp"
#include "util/types.hpp"
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

class Renderer {
public:
    static bool isInvisible();
    static bool isBackground();
    static bool isForeground();
    static int_t getState();
    static void exit();

    virtual void visible() { }
    virtual void background() { }
    virtual void foreground() { }
    virtual void invisible() { }

    virtual void change(int_t w, int_t h) { }
    virtual void drawFrame() { }

    virtual void pointer(PointerEvent event) { }

    virtual void rawPointer(int_t dx, int_t dy) { }
    virtual void rawKey(uint32_t keycode, bool down) { }
    virtual void rawMouseKey(uint32_t key, bool down) { }
    //ticks > 0 => up
    virtual void rawMouseWheel(int_t ticks) { }

    virtual ~Renderer() { }
};
