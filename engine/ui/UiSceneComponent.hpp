#pragma once

#include "ui/UiInput.hpp"
#include "ui/UiScene.hpp"
#include <list>

struct UiRendererClickable {
    UiComponent* comp;
    int id;
};

class UiSceneComponent : public UiComponent {
public:
    UiScene scene;
    std::list<UiRendererClickable> clickables;
    UiSceneComponent();
    void makeClickable(UiComponent& comp, int id);
    void updateRect(ScreenMetrics&, Context<GAllocator*> = {}) override;
    void draw(Framebuffer&, Context<GAllocator*> = {}) override { }
    virtual void pointer(PointerEvent event);
    virtual void onClick(int) { }
};
