#include "UiScene.hpp"

#define GET_UI_BUILDER(comp) ((UiScene*)scene)->layout(comp)
#define GET_BUILDER(comp) &(GET_UI_BUILDER(comp).builder)

UiSceneBuilder& UiScene::layout(UiComponent& comp){
    return builders[&comp];
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
