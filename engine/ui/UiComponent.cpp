#include "ui/UiComponent.hpp"

TypedSizeXY UiComponent_getContentWidth(LazyTypedSizeXY *thiz);
TypedSizeXY UiComponent_getContentHeight(LazyTypedSizeXY *thiz);

LazyTypedSizeXY UiComponent::getContentWidthLazy(){
    LazyTypedSizeXY getter;
    getter.data.ptr = this;
    getter.getSize = UiComponent_getContentWidth;
    return getter;
}

LazyTypedSizeXY UiComponent::getContentHeightLazy(){
    LazyTypedSizeXY getter;
    getter.data.ptr = this;
    getter.getSize = UiComponent_getContentHeight;
    return getter;
}

TypedSizeXY UiComponent_getContentWidth(LazyTypedSizeXY *thiz){
    UiComponent *uc = (UiComponent*)thiz->data.ptr;
    return uc->getContentWidth();
}

TypedSizeXY UiComponent_getContentHeight(LazyTypedSizeXY *thiz){
    UiComponent *uc = (UiComponent*)thiz->data.ptr;
    return uc->getContentHeight();
}
