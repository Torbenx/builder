#ifndef HEADER_UI_UISCENE_H
#define HEADER_UI_UISCENE_H

#include "UiComponent.hpp"
#include "RectBuilder.hpp"
#include <map>

class UiSceneBuilder{
    public:
        void *scene;
        RectBuilder builder;
        UiComponent* comp;
        
        UiSceneBuilder& above(UiComponent& other, TypedSize padding=TypedSize::zero);
        UiSceneBuilder& below(UiComponent& other, TypedSize padding=TypedSize::zero);
        UiSceneBuilder& toLeftOf(UiComponent& other, TypedSize padding=TypedSize::zero);
        UiSceneBuilder& toRightOf(UiComponent& other, TypedSize padding=TypedSize::zero);
        
        UiSceneBuilder& alignTop(UiComponent& other, TypedSize padding=TypedSize::zero);
        UiSceneBuilder& alignBottom(UiComponent& other, TypedSize padding=TypedSize::zero);
        UiSceneBuilder& alignLeft(UiComponent& other, TypedSize padding=TypedSize::zero);
        UiSceneBuilder& alignRight(UiComponent& other, TypedSize padding=TypedSize::zero);
        UiSceneBuilder& alignHorizontal(UiComponent& other, TypedSize padding=TypedSize::zero);
        UiSceneBuilder& alignHorizontal(UiComponent& other, TypedSize paddingLeft, TypedSize paddingRight);
        UiSceneBuilder& alignVertical(UiComponent& other, TypedSize padding=TypedSize::zero);
        UiSceneBuilder& alignVertical(UiComponent& other, TypedSize paddingTop, TypedSize paddingBottom);
        UiSceneBuilder& align(UiComponent& other, TypedSize padding=TypedSize::zero);
        UiSceneBuilder& align(UiComponent& other, TypedSize paddingTop, TypedSize paddingLeft, TypedSize paddingBottom, TypedSize paddingRight);
        
        UiSceneBuilder& alignParentTop(TypedSize padding=TypedSize::zero);
        UiSceneBuilder& alignParentBottom(TypedSize padding=TypedSize::zero);
        UiSceneBuilder& alignParentLeft(TypedSize padding=TypedSize::zero);
        UiSceneBuilder& alignParentRight(TypedSize padding=TypedSize::zero);
        
        UiSceneBuilder& centerHorizontal(float fraction=0.5f);
        UiSceneBuilder& centerVertical(float fraction=0.5f);
        UiSceneBuilder& centerHorizontalInside(UiComponent& other, float fraction=0.5f);
        UiSceneBuilder& centerVerticalInside(UiComponent& other, float fraction=0.5f);
        
        UiSceneBuilder& width(TypedSize width);
        UiSceneBuilder& height(TypedSize height);
        
        UiSceneBuilder& wrapContentWidth(TypedSize padding=TypedSize::zero);
        UiSceneBuilder& wrapContentHeight(TypedSize padding=TypedSize::zero);
        UiSceneBuilder& wrapContent(TypedSize padding=TypedSize::zero);
        
        UiSceneBuilder& wrapContentWidthOf(UiComponent& other, TypedSize padding=TypedSize::zero);
        UiSceneBuilder& wrapContentHeightOf(UiComponent& other, TypedSize padding=TypedSize::zero);
        UiSceneBuilder& wrapContentOf(UiComponent& other, TypedSize padding=TypedSize::zero);
};

class UiScene{
    public:
        ScreenMetrics metrics;
        std::map<UiComponent*, UiSceneBuilder> builders;
        UiSceneBuilder& layout(UiComponent& comp);
        void build();
};

#endif