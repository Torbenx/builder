#include "ui/TypedSize.hpp"
#include <cmath>
#include <math.h>

TypedSize TypedSize::zero = {TYPED_SIZE_GL, 0.0f};

TypedSize TypedSize::nan = {TYPED_SIZE_GL, NAN};

float TypedSize::toPixels(SizeMetrics m){
    switch(type){
        case TYPED_SIZE_PIXELS:
            return value;
        case TYPED_SIZE_GL:
            return 0.5f * m.size * value;
        case TYPED_SIZE_ANDROID_DIP:
            return value * m.dpi / 160.0f;
        case TYPED_SIZE_WINDOWS_DIP:
            return value * m.dpi / 96.0f;
        default:
            return NAN;
    }
}

float TypedSize::toGL(SizeMetrics m){
    switch(type){
        case TYPED_SIZE_PIXELS:
            return value * 2.0f / m.size;
        case TYPED_SIZE_GL:
            return value;
        case TYPED_SIZE_ANDROID_DIP:
            return value * m.dpi / 160.0f * 2.0f / m.size;
        case TYPED_SIZE_WINDOWS_DIP:
            return value * m.dpi / 96.0f * 2.0f / m.size;
        default:
            return NAN;
    }
}

float TypedSize::toAndroidDIP(SizeMetrics m){
    switch(type){
        case TYPED_SIZE_PIXELS:
            return value * 160.0f / m.dpi;
        case TYPED_SIZE_GL:
            return 0.5f * m.size * value * 160.0f / m.dpi;
        case TYPED_SIZE_ANDROID_DIP:
            return value;
        case TYPED_SIZE_WINDOWS_DIP:
            return value / 96.0f * 160.0f;
        default:
            return NAN;
    }
}

float TypedSize::toWindowsDIP(SizeMetrics m){
    switch(type){
        case TYPED_SIZE_PIXELS:
            return value * 96.0f / m.dpi;
        case TYPED_SIZE_GL:
            return 0.5f * m.size * value * 96.0f / m.dpi;
        case TYPED_SIZE_ANDROID_DIP:
            return value * 96.0f / 160.0f;
        case TYPED_SIZE_WINDOWS_DIP:
            return value;
        default:
            return NAN;
    }
}

float TypedSize::to(TYPED_SIZE_TYPE type, SizeMetrics m){
    switch(type){
        case TYPED_SIZE_PIXELS:
            return toPixels(m);
        case TYPED_SIZE_GL:
            return toGL(m);
        case TYPED_SIZE_ANDROID_DIP:
            return toAndroidDIP(m);
        case TYPED_SIZE_WINDOWS_DIP:
            return toWindowsDIP(m);
        default:
            return NAN;
    }
}

TypedSizeXY TypedSize::asXSize(){
    TypedSizeXY r;
    r.isX = true;
    r.size = *this;
    return r;
}

TypedSizeXY TypedSize::asYSize(){
    TypedSizeXY r;
    r.isX = false;
    r.size = *this;
    return r;
}

float TypedSizeXY::toPixels(ScreenMetrics metrics){
    return size.toPixels(isX ? metrics.x : metrics.y);
}

float TypedSizeXY::toGL(ScreenMetrics metrics){
    return size.toGL(isX ? metrics.x : metrics.y);
}

float TypedSizeXY::toAndroidDIP(ScreenMetrics metrics){
    return size.toAndroidDIP(isX ? metrics.x : metrics.y);
}

float TypedSizeXY::toWindowsDIP(ScreenMetrics metrics){
    return size.toWindowsDIP(isX ? metrics.x : metrics.y);
}

float TypedSizeXY::to(TYPED_SIZE_TYPE type, ScreenMetrics metrics){
    return size.to(type, isX ? metrics.x : metrics.y);
}

TypedSize TypedSize::pixels(float f){
    return {TYPED_SIZE_PIXELS, f};
}

TypedSize TypedSize::gl(float f){
    return {TYPED_SIZE_GL, f};
}

TypedSize TypedSize::androidDIP(float f){
    return {TYPED_SIZE_ANDROID_DIP, f};
}

TypedSize TypedSize::windowsDIP(float f){
    return {TYPED_SIZE_WINDOWS_DIP, f};
}

TypedSize& TypedSize::operator*=(float f){
    value *= f;
    return *this;
}

TypedSize operator-(TypedSize size){
    size.value *= -1.0f;
    return size;
}

TypedSize operator*(float left, TypedSize right){
    right.value *= left;
    return right;
}

TypedSize operator*(TypedSize left, float right){
    left.value *= right;
    return left;
}

TypedSizeXY TypedSizeXY_getSizeLazy(LazyTypedSizeXY *thiz){
    return thiz->data.size;
}

LazyTypedSizeXY TypedSizeXY::lazy(){
    LazyTypedSizeXY r;
    r.data.size = *this;
    r.getSize = TypedSizeXY_getSizeLazy;
    return r;
}

TypedSizeXY LazyTypedSizeXY::get(){
    return getSize(this);
}
