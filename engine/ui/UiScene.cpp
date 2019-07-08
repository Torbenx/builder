#include "UiScene.hpp"
#include "../util/log.hpp"

#define GET_UI_BUILDER(comp) ((UiScene*)scene)->layout(comp)
#define GET_BUILDER(comp) &(GET_UI_BUILDER(comp).builder)

UiSceneBuilder& UiScene::layout(UiComponent& comp){
    UiSceneBuilder& builder = builders[&comp];
    builder.scene = this;
    builder.comp = &comp;
    return builder;
}

void UiScene::build(){
    for(auto it=builders.begin();it!=builders.end();it++){
        it->first->metrics = metrics;
        it->first->rect = it->second.builder.build(metrics);
        it->first->updateRect();
    }
}


UiSceneBuilder& UiSceneBuilder::above(UiComponent& other, TypedSize padding){
   builder.above(GET_BUILDER(other), padding);
   return *this;
}

UiSceneBuilder& UiSceneBuilder::below(UiComponent& other, TypedSize padding){
	builder.below(GET_BUILDER(other), padding);
    return *this;
}

UiSceneBuilder& UiSceneBuilder::toLeftOf(UiComponent& other, TypedSize padding){
    builder.toLeftOf(GET_BUILDER(other), padding);
    return *this;
}

UiSceneBuilder& UiSceneBuilder::toRightOf(UiComponent& other, TypedSize padding){
    builder.toRightOf(GET_BUILDER(other), padding);
    return *this;
}


UiSceneBuilder& UiSceneBuilder::alignTop(UiComponent& other, TypedSize padding){
    builder.alignTop(GET_BUILDER(other), padding);
    return *this;
}

UiSceneBuilder& UiSceneBuilder::alignBottom(UiComponent& other, TypedSize padding){
    builder.alignBottom(GET_BUILDER(other), padding);
    return *this;
}

UiSceneBuilder& UiSceneBuilder::alignLeft(UiComponent& other, TypedSize padding){
    builder.alignLeft(GET_BUILDER(other), padding);
    return *this;
}

UiSceneBuilder& UiSceneBuilder::alignRight(UiComponent& other, TypedSize padding){
    builder.alignRight(GET_BUILDER(other), padding);
    return *this;
}

UiSceneBuilder& UiSceneBuilder::alignHorizontal(UiComponent& other, TypedSize padding){
    RectBuilder *ob = GET_BUILDER(other);
    builder.alignLeft(ob, padding);
    builder.alignRight(ob, padding);
    return *this;
}

UiSceneBuilder& UiSceneBuilder::alignHorizontal(UiComponent& other, TypedSize paddingLeft, TypedSize paddingRight){
    RectBuilder *ob = GET_BUILDER(other);
    builder.alignLeft(ob, paddingLeft);
    builder.alignRight(ob, paddingRight);
    return *this;
}

UiSceneBuilder& UiSceneBuilder::alignVertical(UiComponent& other, TypedSize padding){
    RectBuilder *ob = GET_BUILDER(other);
    builder.alignTop(ob, padding);
    builder.alignBottom(ob, padding);
    return *this;
}

UiSceneBuilder& UiSceneBuilder::alignVertical(UiComponent& other, TypedSize paddingTop, TypedSize paddingBottom){
    RectBuilder *ob = GET_BUILDER(other);
    builder.alignTop(ob, paddingTop);
    builder.alignBottom(ob, paddingBottom);
    return *this;
}

UiSceneBuilder& UiSceneBuilder::align(UiComponent& other, TypedSize padding){
    RectBuilder *ob = GET_BUILDER(other);
    builder.alignLeft(ob, padding);
    builder.alignRight(ob, padding);
    builder.alignTop(ob, padding);
    builder.alignBottom(ob, padding);
    return *this;
}

UiSceneBuilder& UiSceneBuilder::align(UiComponent& other, TypedSize paddingTop, TypedSize paddingLeft, TypedSize paddingBottom, TypedSize paddingRight){
    RectBuilder *ob = GET_BUILDER(other);
    builder.alignLeft(ob, paddingLeft);
    builder.alignRight(ob, paddingRight);
    builder.alignTop(ob, paddingTop);
    builder.alignBottom(ob, paddingBottom);
    return *this;
}


UiSceneBuilder& UiSceneBuilder::alignParentTop(TypedSize padding){
    builder.alignTop(nullptr, padding);
    return *this;
}

UiSceneBuilder& UiSceneBuilder::alignParentBottom(TypedSize padding){
    builder.alignBottom(nullptr, padding);
    return *this;
}

UiSceneBuilder& UiSceneBuilder::alignParentLeft(TypedSize padding){
    builder.alignLeft(nullptr, padding);
    return *this;
}

UiSceneBuilder& UiSceneBuilder::alignParentRight(TypedSize padding){
    builder.alignRight(nullptr, padding);
    return *this;
}


UiSceneBuilder& UiSceneBuilder::centerHorizontal(float fraction){
    builder.centerHorizontalInside(nullptr, fraction);
    return *this;
}

UiSceneBuilder& UiSceneBuilder::centerVertical(float fraction){
    builder.centerVerticalInside(nullptr, fraction);
    return *this;
}

UiSceneBuilder& UiSceneBuilder::centerHorizontalInside(UiComponent& other, float fraction){
    builder.centerHorizontalInside(GET_BUILDER(other), fraction);
    return *this;
}

UiSceneBuilder& UiSceneBuilder::centerVerticalInside(UiComponent& other, float fraction){
    builder.centerVerticalInside(GET_BUILDER(other), fraction);
    return *this;
}


UiSceneBuilder& UiSceneBuilder::width(TypedSize width){
    builder.width(width);
    return *this;
}

UiSceneBuilder& UiSceneBuilder::height(TypedSize height){
    builder.height(height);
    return *this;
}


UiSceneBuilder& UiSceneBuilder::wrapContentWidth(TypedSize padding){
    builder.width(comp->contentWidthGetter, padding);
    return *this;
}

UiSceneBuilder& UiSceneBuilder::wrapContentHeight(TypedSize padding){
    builder.height(comp->contentHeightGetter, padding);
    return *this;
}

UiSceneBuilder& UiSceneBuilder::wrapContent(TypedSize padding){
    wrapContentWidth(padding);
    wrapContentHeight(padding);
    return *this;
}


UiSceneBuilder& UiSceneBuilder::wrapContentWidthOf(UiComponent& other, TypedSize padding){
    builder.width(other.contentWidthGetter, padding);
    return *this;
}

UiSceneBuilder& UiSceneBuilder::wrapContentHeightOf(UiComponent& other, TypedSize padding){
    builder.height(other.contentHeightGetter, padding);
    return *this;
}

UiSceneBuilder& UiSceneBuilder::wrapContentOf(UiComponent& other, TypedSize padding){
    wrapContentWidthOf(other, padding);
    wrapContentHeightOf(other, padding);
    return *this;
}
