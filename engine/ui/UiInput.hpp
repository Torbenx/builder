#pragma once

#include "input/Pointer.hpp"
#include "ui/TypedPoint.hpp"

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
