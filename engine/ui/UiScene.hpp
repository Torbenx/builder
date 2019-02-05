#ifndef HEADER_UI_UISCENE_H
#define HEADER_UI_UISCENE_H

class UiSceneBuilder{
    public:
        void *scene;
        RectBuilder rect;
        
        UiSceneBuilder& above(UiComponent& other, TypedSize padding=TypedSize::zero);
        UiSceneBuilder& below(UiComponent& other, TypedSize padding=TypedSize::zero);
        UiSceneBuilder& toLeftOf(UiComponent& other, TypedSize padding=TypedSize::zero);
        UiSceneBuilder& toRightOf(UiComponent& other, TypedSize padding=TypedSize::zero);
        
        UiSceneBuilder& alignTop(UiComponent& other, TypedSize padding=TypedSize::zero);
        UiSceneBuilder& alignBottom(UiComponent& other, TypedSize padding=TypedSize::zero);
        UiSceneBuilder& alignLeft(UiComponent& other, TypedSize padding=TypedSize::zero);
        UiSceneBuilder& alignBottom(UiComponent& other, TypedSize padding=TypedSize::zero);
        
        UiSceneBuilder& width(TypedSize padding=TypedSize::zero);
        UiSceneBuilder& height(TypedSize padding=TypedSize::zero);
        
        UiSceneBuilder& wrapContentWidth(TypedSize paddingLeft, TypedSize paddingRight);
        UiSceneBuilder& wrapContentWidth(TypedSize padding=TypedSize::zero);
        UiSceneBuilder& wrapContentHeight(TypedSize paddingTop, TypedSize paddingBottom);
        UiSceneBuilder& wrapContentHeight(TypedSize padding=TypedSize::zero);
        UiSceneBuilder& wrapContent(TypedSize padding=TypedSize::zero);
        
        /*UiSceneBuilder& wrapHorizontal(UiComponent& other, float paddingLeft, float paddingRight);
        UiSceneBuilder& wrapHorizontal(UiComponent& other, float padding=0.0f);
        UiSceneBuilder& wrapVertical(UiComponent& other, float paddingTop, float paddingBottom);
        UiSceneBuilder& wrapVertical(UiComponent& other, float paddingTop, float paddingBottomfloat padding=0.0f);
        UiSceneBuilder& wrap(UiComponent& other, float padding=0.0f);*/
};

class UiScene{
    public:
        ScreenMetrics metrics;
        std::map<UiComponent*, UiSceneBuilder>
        UiSceneBuilder& add(UiComponent& comp);
        UiSceneBuilder& layout(UiComponent& comp);
};

#endif