#pragma once

#include "input/Pointer.hpp"
#include "util/Gapi.hpp"
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

struct RendererState {
    static bool isInvisible();
    static bool isBackground();
    static bool isForeground();
    static int_t getState();
    static void exit();
};

template<Gapi G>
class Renderer {
public:
    virtual void visible() { }
    virtual void background() { }
    virtual void foreground() { }
    virtual void invisible() { }

    virtual void change([[maybe_unused]] int_t w, [[maybe_unused]] int_t h) { }
    virtual void drawFrame() { }

    virtual void pointer([[maybe_unused]] PointerEvent event) { }

    virtual void rawPointer([[maybe_unused]] int_t dx, [[maybe_unused]] int_t dy) { }
    virtual void rawKey([[maybe_unused]] uint32_t keycode, [[maybe_unused]] bool down) { }
    virtual void rawMouseKey([[maybe_unused]] uint32_t key, [[maybe_unused]] bool down) { }
    //ticks > 0 => up
    virtual void rawMouseWheel([[maybe_unused]] int_t ticks) { }

    virtual ~Renderer() { }
};

#define __FOREACH_G_API(api) Renderer<api>*(*createRenderer_##api)();
struct CreateFunctions {
    ENUMERATE_G_APIS(__FOREACH_G_API)
};
#undef __FOREACH_G_API
