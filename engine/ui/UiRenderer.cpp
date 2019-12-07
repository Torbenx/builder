#include "UiRenderer.hpp"
#include "../fullscreen.hpp"

UiRenderer::UiRenderer(){
    
}

void UiRenderer::makeClickable(UiComponent& comp, int id){
    UiRendererClickable clickable;
    clickable.comp = &comp;
    clickable.id = id;
    clickables.push_back(clickable);
}

void UiRenderer::change(int newWidth, int newHeight){
    ScreenMetrics metrics;
    int dpi = getDPI();
    metrics.x.size = newWidth;
    metrics.x.dpi = dpi;
    metrics.y.size = newHeight;
    metrics.y.dpi = dpi;
    scene.metrics = metrics;
    onChange(metrics);
    scene.build();
}

void UiRenderer::pointer(PointerEvent event){
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