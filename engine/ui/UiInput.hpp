#ifndef HEADER_UI_UI_INPUT_H
#define HEADER_UI_UI_INPUT_H

#include "../input/Pointer.hpp"
#include "TypedPoint.hpp"

#ifdef PLATFORM_WINDOWS
#define POINTER_EVENT_POINT_TYPE TYPED_POINT_TOP_LOW_PIXELS
#elif PLATFORM_ANDROID
#define POINTER_EVENT_POINT_TYPE TYPED_POINT_TOP_LOW_PIXELS
#endif

struct UiPointerEvent{
    UiPointerEvent(PointerEvent ev);
    int pointerId;
    int event;
    TypedPoint p;
};

class UiInputProcessor{
    public:
        void event(UiPointerEvent ev);
};

#endif