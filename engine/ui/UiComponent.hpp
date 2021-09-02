#pragma once

#include "GL/Framebuffer.hpp"
#include "math/Rect.hpp"
#include "ui/TypedSize.hpp"
#include "ui/UiSolver.hpp"
#include "graphics/GData.hpp"
#include <cmath>

class UiComponent {
public:
    Rect rect;
    ui_var_t base = UI_VAR_INVALID;
    UiComponent() { }
    UiComponent(const UiComponent&) = delete;

    virtual void updateContent(ScreenMetrics&, GAllocator&) { }
    virtual TypedSizeXY getContentWidth() { return TypedSize::nan.asXSize(); }
    virtual TypedSizeXY getContentHeight() { return TypedSize::nan.asYSize(); }
    virtual void updateRect(ScreenMetrics&, GAllocator&) = 0;
    virtual void draw(Framebuffer&, GAllocator&) = 0;
    virtual void detach(GAllocator&) { }
    virtual ~UiComponent() { }

    LazyTypedSizeXY getContentWidthLazy();
    LazyTypedSizeXY getContentHeightLazy();
};
