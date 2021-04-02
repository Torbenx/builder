#pragma once

#include "ui/UiScene.hpp"
#include "ui/UiInput.hpp"
#include <list>

struct UiRendererClickable{
    UiComponent *comp;
    int id;
};

class UiSceneComponent : public UiComponent{
    public:
        UiScene scene;
        std::list<UiRendererClickable> clickables;
        UiSceneComponent();
        void makeClickable(UiComponent& comp, int id);
        void updateRect(ScreenMetrics&);
        void draw(Framebuffer&){}
        virtual void pointer(PointerEvent event);
        virtual void onClick(int id){}
};
