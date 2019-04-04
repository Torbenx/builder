#ifndef HEADER_UI_UISCENE_H
#define HEADER_UI_UISCENE_H

class UiSceneBuilder{
    public:
        void *scene;
        RectBuilder builder;
        
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
        std::map<UiComponent*, UiSceneBuilder>
        UiSceneBuilder& add(UiComponent& comp);
        UiSceneBuilder& layout(UiComponent& comp);
};

#endif