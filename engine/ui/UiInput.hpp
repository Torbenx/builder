#pragma once

#include "input/Pointer.hpp"
#include "ui/TypedPoint.hpp"
#include "util/target.hpp"

#if defined(PLATFORM_WINDOWS)
#define POINTER_EVENT_POINT_TYPE TYPED_POINT_TOP_LOW_PIXELS
#elif defined(PLATFORM_ANDROID)
#define POINTER_EVENT_POINT_TYPE TYPED_POINT_TOP_LOW_PIXELS
#endif

struct UiPointerEvent{
    UiPointerEvent(PointerEvent ev);
    int pointerId;
    int event;
    TypedPoint p;
};
