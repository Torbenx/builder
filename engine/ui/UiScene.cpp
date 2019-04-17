#include "UiScene.hpp"

UiSceneBuilder& UiSceneBuilder::above(UiComponent& other, TypedSize padding ){
   
}

UiSceneBuilder& UiSceneBuilder::below(UiComponent& other, TypedSize padding ){
	
}

UiSceneBuilder& UiSceneBuilder::toLeftOf(UiComponent& other, TypedSize padding ){
    
}

UiSceneBuilder& UiSceneBuilder::toRightOf(UiComponent& other, TypedSize padding ){
    
}


UiSceneBuilder& UiSceneBuilder::alignTop(UiComponent& other, TypedSize padding ){}
UiSceneBuilder& UiSceneBuilder::alignBottom(UiComponent& other, TypedSize padding ){}
UiSceneBuilder& UiSceneBuilder::alignLeft(UiComponent& other, TypedSize padding ){}
UiSceneBuilder& UiSceneBuilder::alignRight(UiComponent& other, TypedSize padding ){}
UiSceneBuilder& UiSceneBuilder::alignHorizontal(UiComponent& other, TypedSize padding ){}
UiSceneBuilder& UiSceneBuilder::alignHorizontal(UiComponent& other, TypedSize paddingLeft, TypedSize paddingRight){}
UiSceneBuilder& UiSceneBuilder::alignVertical(UiComponent& other, TypedSize padding ){}
UiSceneBuilder& UiSceneBuilder::alignVertical(UiComponent& other, TypedSize paddingTop, TypedSize paddingBottom){}
UiSceneBuilder& UiSceneBuilder::align(UiComponent& other, TypedSize padding ){}
UiSceneBuilder& UiSceneBuilder::align(UiComponent& other, TypedSize paddingTop, TypedSize paddingLeft, TypedSize paddingBottom, TypedSize paddingRight){}

UiSceneBuilder& UiSceneBuilder::width(TypedSize width){}
UiSceneBuilder& UiSceneBuilder::height(TypedSize height){}

UiSceneBuilder& UiSceneBuilder::wrapContentWidth(TypedSize padding ){}
UiSceneBuilder& UiSceneBuilder::wrapContentHeight(TypedSize padding ){}
UiSceneBuilder& UiSceneBuilder::wrapContent(TypedSize padding ){}

UiSceneBuilder& UiSceneBuilder::wrapContentWidthOf(UiComponent& other, TypedSize padding ){}
UiSceneBuilder& UiSceneBuilder::wrapContentHeightOf(UiComponent& other, TypedSize padding ){}
UiSceneBuilder& UiSceneBuilder::wrapContentOf(UiComponent& other, TypedSize padding ){}
