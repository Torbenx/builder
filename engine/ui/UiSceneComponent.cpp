#include "UiSceneComponent.hpp"
#include "../fullscreen.hpp"

UiSceneComponent::UiSceneComponent(){
    
}

void UiSceneComponent::makeClickable(UiComponent& comp, int id){
    UiRendererClickable clickable;
    clickable.comp = &comp;
    clickable.id = id;
    clickables.push_back(clickable);
}

void UiSceneComponent::updateRect(){
    scene.metrics = metrics;
    scene.setParent(rect);
    scene.build();
}

void UiSceneComponent::pointer(PointerEvent event){
    UiPointerEvent ev(event);
    std::list<int> clicks;
    if(ev.event == POINTER_EVENT_UP){
        for(auto it=clickables.begin();it!=clickables.end();it++){
            if(it->comp->rect.contains(ev.p.toGL(scene.metrics))){
                clicks.push_back(it->id);
            }
        }
    }
    for(auto it=clicks.begin();it!=clicks.end();it++){
        onClick(*it);
    }
}