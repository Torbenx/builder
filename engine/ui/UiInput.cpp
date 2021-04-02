#include "ui/UiInput.hpp"

UiPointerEvent::UiPointerEvent(PointerEvent ev){
    pointerId = ev.pointerId;
    event = ev.event;
    p = {POINTER_EVENT_POINT_TYPE, Vec2(ev.x, ev.y)};
}
