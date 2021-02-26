#ifndef HEADER_UI_UISCENE_H
#define HEADER_UI_UISCENE_H

#include "UiComponent.hpp"
#include "UiSolver.hpp"
#include <vector>

//match Rect
#define UI_VAR_TOP 0
#define UI_VAR_BOTTOM 1
#define UI_VAR_LEFT 2
#define UI_VAR_RIGHT 3

class UiSceneBuilder; 

class UiScene{
    public:
        ScreenMetrics metrics;
        Rect parent = Rect::bltr(-1.0f, -1.0f, 1.0f, 1.0f);
        std::vector<UiComponent*> components;
        UiSolver solver;
        
        UiScene();
        void setParent(Rect rect);
        UiSceneBuilder layout(UiComponent* comp);
        void build();
        void clear();
};

class UiSceneBuilder{
    public:
        UiScene* scene;
        UiComponent* comp;
        
        ui_var_t top(){return (comp ? comp->base : 0) + UI_VAR_TOP;}
        ui_var_t bottom(){return (comp ? comp->base : 0) + UI_VAR_BOTTOM;}
        ui_var_t left(){return (comp ? comp->base : 0) + UI_VAR_LEFT;}
        ui_var_t right(){return (comp ? comp->base : 0) + UI_VAR_RIGHT;}
        
        UiSceneBuilder& above(UiComponent* other=nullptr, TypedSize padding=TypedSize::zero);
        UiSceneBuilder& below(UiComponent* other=nullptr, TypedSize padding=TypedSize::zero);
        UiSceneBuilder& toLeftOf(UiComponent* other=nullptr, TypedSize padding=TypedSize::zero);
        UiSceneBuilder& toRightOf(UiComponent* other=nullptr, TypedSize padding=TypedSize::zero);
        
        UiSceneBuilder& alignTop(UiComponent* other=nullptr, TypedSize padding=TypedSize::zero);
        UiSceneBuilder& alignBottom(UiComponent* other=nullptr, TypedSize padding=TypedSize::zero);
        UiSceneBuilder& alignLeft(UiComponent* other=nullptr, TypedSize padding=TypedSize::zero);
        UiSceneBuilder& alignRight(UiComponent* other=nullptr, TypedSize padding=TypedSize::zero);
        UiSceneBuilder& alignHorizontal(UiComponent* other=nullptr, TypedSize padding=TypedSize::zero);
        UiSceneBuilder& alignHorizontal(UiComponent* other, TypedSize paddingLeft, TypedSize paddingRight);
        UiSceneBuilder& alignVertical(UiComponent* other=nullptr, TypedSize padding=TypedSize::zero);
        UiSceneBuilder& alignVertical(UiComponent* other, TypedSize paddingTop, TypedSize paddingBottom);
        UiSceneBuilder& align(UiComponent* other=nullptr, TypedSize padding=TypedSize::zero);
        UiSceneBuilder& align(UiComponent* other, TypedSize paddingTop, TypedSize paddingLeft, TypedSize paddingBottom, TypedSize paddingRight);
        
        UiSceneBuilder& centerHorizontal(UiComponent* other=nullptr, float fraction=0.5f);
        UiSceneBuilder& centerVertical(UiComponent* other=nullptr, float fraction=0.5f);
        
        UiSceneBuilder& width(TypedSize width);
        UiSceneBuilder& height(TypedSize height);
        
        UiSceneBuilder& wrapContentWidth();
        UiSceneBuilder& wrapContentHeight();
        UiSceneBuilder& wrapContent();
};

#endif