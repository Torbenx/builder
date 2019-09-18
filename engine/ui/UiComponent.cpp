#include "UiComponent.hpp"

TypedSize UiComponent_getContentWidth(TypedSizeGetter *thiz);
TypedSize UiComponent_getContentHeight(TypedSizeGetter *thiz);

TypedSizeGetter UiComponent::getContentWidthGetter(){
    TypedSizeGetter getter;
    getter.data = this;
    getter.getSize = UiComponent_getContentWidth;
    return getter;
}

TypedSizeGetter UiComponent::getContentHeightGetter(){
    TypedSizeGetter getter;
    getter.data = this;
    getter.getSize = UiComponent_getContentHeight;
    return getter;
}

TypedSize UiComponent_getContentWidth(TypedSizeGetter *thiz){
    UiComponent *uc = (UiComponent*)thiz->data;
    return uc->getContentWidth();
}

TypedSize UiComponent_getContentHeight(TypedSizeGetter *thiz){
    UiComponent *uc = (UiComponent*)thiz->data;
    return uc->getContentHeight();
}