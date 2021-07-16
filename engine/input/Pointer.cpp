#include "input/Pointer.hpp"
#include "util/log.hpp"
#include "util/target.hpp"
#include <vector>
#ifdef PLATFORM_WINDOWS
#include <Windows.h>
#endif

std::vector<Pointer> pointers;

void trackPointerEvent(PointerEvent& event) {
    switch (event.event) {
    case POINTER_EVENT_UP: {
        bool found = false;
        for (std::vector<Pointer>::iterator it = pointers.begin(); it != pointers.end(); it++) {
            if (it->pointerId == event.pointerId) {
                if (it->isDown) {
                    it->isDown = false;
                } else {
                    log("up of pointer that was already up");
                }
                found = true;
                break;
            }
        }
        if (!found) {
            log("up of unknown pointer");
        }
        break;
    }
    case POINTER_EVENT_DOWN:
    case POINTER_EVENT_MOVE: {
        bool found = false;
        for (std::vector<Pointer>::iterator it = pointers.begin(); it != pointers.end(); it++) {
            if (it->pointerId == event.pointerId) {
                it->x = event.x;
                it->y = event.y;
                if (event.event == POINTER_EVENT_DOWN) {
                    if (it->isDown) {
                        log("down of pointer that was already down");
                    } else {
                        it->isDown = true;
                    }
                }
                found = true;
                break;
            }
        }
        if (!found) {
            Pointer pointer;
            pointer.pointerId = event.pointerId;
            pointer.isDown = event.event == POINTER_EVENT_DOWN;
            pointer.x = event.x;
            pointer.y = event.y;
            pointers.push_back(pointer);
#ifdef PLATFORM_ANDROID
            if (event.event != POINTER_EVENT_DOWN) {
                log("pointer moving that isn't down");
            }
#endif
        }
        break;
    }
    }
}

Pointer getPointerById(int pointerId, bool mustBeDown) {
    for (Pointer pointer : pointers) {
        if (pointer.pointerId == pointerId) {
            if (mustBeDown && !pointer.isDown) {
                break;
            }
            return pointer;
        }
    }
    Pointer pointer;
    pointer.pointerId = POINTER_ID_INVALID;
    return pointer;
}

Pointer getPointerByIndex(int index, bool mustBeDown) {
    Pointer pointer = pointers[index];
    if (mustBeDown && !pointer.isDown) {
        Pointer pointer;
        pointer.pointerId = POINTER_ID_INVALID;
        return pointer;
    }
    return pointer;
}

int getPointerCount(bool mustBeDown) {
    if (!mustBeDown) {
        return pointers.size();
    } else {
        int count = 0;
        for (std::vector<Pointer>::iterator it = pointers.begin(); it != pointers.end(); it++) {
            if (it->isDown)
                count++;
        }
        return count;
    }
}

/*void setCursorVisible(bool visible) {
}*/

PointerEvent teleportCursor([[maybe_unused]] float x, [[maybe_unused]] float y) {
    PointerEvent event;
    event.event = POINTER_EVENT_MOVE;
    event.pointerId = POINTER_ID_INVALID;
#ifdef PLATFORM_WINDOWS
    event.pointerId = POINTER_ID_CURSOR;
    event.x = x;
    event.y = y;
    SetCursorPos(x, y);
#endif
    return event;
}
