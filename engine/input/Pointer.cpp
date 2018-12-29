#include "Pointer.hpp"
#include "../util/log.hpp"
#include <vector>

Pointer Pointer::operator=(Pointer pointer){
    this->pointerId = pointer.pointerId;
    this->x = pointer.x;
    this->y = pointer.y;
    return *this;
}

std::vector<Pointer> pointers;

void trackPointerEvent(PointerEvent& event){
    switch(event.event){
        case POINTER_EVENT_UP:{
            bool found = false;
            for(std::vector<Pointer>::iterator it=pointers.begin();it!=pointers.end();it++){
                if(it->pointerId == event.pointerId){
                    pointers.erase(it);
                    found = true;
                    break;
                }
            }
            if(!found){
                LOGW("up of unknown pointer");
            }
            break;
        }
        case POINTER_EVENT_DOWN:
        case POINTER_EVENT_MOVE:
        {
            bool found = false;
            for(std::vector<Pointer>::iterator it=pointers.begin();it!=pointers.end();it++){
                if(it->pointerId == event.pointerId){
                    it->x = event.x;
                    it->y = event.y;
                    if(event.event == POINTER_EVENT_DOWN){
                        LOGW("down of pointer that was already down");
                    }
                    found = true;
                    break;
                }
            }
            if(!found){
                Pointer pointer;
                pointer.pointerId = event.pointerId;
                pointer.x = event.x;
                pointer.y = event.y;
                pointers.push_back(pointer);
                #ifdef PLATFORM_ANDROID
                if(event.event != POINTER_EVENT_DOWN){
                    LOGW("pointer moving that wasn't down");
                }
                #endif
            }
            break;
        }
    }
}

Pointer getPointerById(int pointerId){
    for(Pointer pointer : pointers){
        if(pointer.pointerId == pointerId){
            return pointer;
        }
    }
    Pointer pointer;
    pointer.pointerId = POINTER_ID_INVALID;
    return pointer;
}

Pointer getPointerByIndex(int index){
    return pointers[index];
}

int getPointerCount(){
    return pointers.size();
}

#ifdef PLATFORM_WINDOWS
#include "../windows/engine.hpp"
#include "../graphic/Framebuffer.hpp"
#endif

void setCursorVisible(bool visible){
    #ifdef PLATFORM_WINDOWS
    ShowCursor(visible);
    #endif
}

PointerEvent teleportCursor(float x, float y){
    PointerEvent event;
    event.event = POINTER_EVENT_MOVE;
    event.pointerId = POINTER_ID_INVALID;
    #ifdef PLATFORM_WINDOWS
    RECT rect;
    GetWindowRect(engine_hwnd, &rect);
    event.pointerId = POINTER_ID_CURSOR;
    event.x = rect.left + Framebuffer::DEFAULT.offsetX + (x + 1.0f) * 0.5f * Framebuffer::DEFAULT.width;
    event.y = rect.top  + Framebuffer::DEFAULT.offsetY + (y + 1.0f) * 0.5f * Framebuffer::DEFAULT.height;
    SetCursorPos(event.x, event.y);
    #endif
    return event;
}