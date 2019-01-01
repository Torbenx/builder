#ifndef HEADER_INPUT_POINTER_H
#define HEADER_INPUT_POINTER_H

#define POINTER_EVENT_DOWN 1
#define POINTER_EVENT_MOVE 2
#define POINTER_EVENT_UP 3

#define POINTER_ID_INVALID (-1)

#ifdef PLATFORM_WINDOWS
#define POINTER_ID_CURSOR 0
#endif


struct PointerEvent{
    int pointerId;
    int event;
    int x,y;
};

struct Pointer{
    int pointerId;
    int x,y;
    Pointer operator=(Pointer pointer);
};

void trackPointerEvent(PointerEvent& event);
Pointer getPointerById(int pointerId);
Pointer getPointerByIndex(int index);
int getPointerCount();
void setCursorVisible(bool visible);
PointerEvent teleportCursor(float x, float y);

#endif