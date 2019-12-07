#ifndef HEADER_UI_RENDERER_H
#define HEADER_UI_RENDERER_H

#include "UiScene.hpp"
#include <list>
#include "UiInput.hpp"

struct UiRendererClickable{
    UiComponent *comp;
    int id;
};

class UiRenderer{
    public:
        UiScene scene;
        std::list<UiRendererClickable> clickables;
        UiRenderer();
        void makeClickable(UiComponent& comp, int id);
        virtual void change(int w,int h);
        virtual void onChange(ScreenMetrics metrics){}
        virtual void drawFrame(){}
        virtual void pointer(PointerEvent event);
        virtual void onClick(int id){}
        virtual ~UiRenderer(){}
};

#endif