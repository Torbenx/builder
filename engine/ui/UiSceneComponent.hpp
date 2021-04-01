#ifndef HEADER_UI_SCENE_COMPONENT_H
#define HEADER_UI_SCENE_COMPONENT_H

#include "UiScene.hpp"
#include <list>
#include "UiInput.hpp"

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
        void updateRect();
        void draw(Framebuffer&){}
        virtual void pointer(PointerEvent event);
        virtual void onClick(int id){}
};

#endif
