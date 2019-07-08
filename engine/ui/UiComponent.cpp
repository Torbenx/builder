#include "UiComponent.hpp"

TypedSize UiComponent_getContentWidth(TypedSizeGetter *thiz);
TypedSize UiComponent_getContentHeight(TypedSizeGetter *thiz);

UiComponent::UiComponent(){
    contentWidthGetter.data = this;
    contentWidthGetter.getSize = UiComponent_getContentWidth;
    contentHeightGetter.data = this;
    contentHeightGetter.getSize = UiComponent_getContentHeight;
}

TypedSize UiComponent_getContentWidth(TypedSizeGetter *thiz){
    UiComponent *uc = (UiComponent*)thiz->data;
    return uc->getContentWidth();
}

TypedSize UiComponent_getContentHeight(TypedSizeGetter *thiz){
    UiComponent *uc = (UiComponent*)thiz->data;
    return uc->getContentHeight();
}