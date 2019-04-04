#include "TypedSize.hpp"
#include <cmath>
#include <math.h>

TypedSize TypedSize::zero = {TYPED_SIZE_GL, 0.0f};

TypedSize TypedSize::nan = {TYPED_SIZE_GL, NAN};

float TypedSize::toPixelsF(SizeMetrics m){
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

int TypedSize::toPixels(SizeMetrics m){
    float f = toPixelsF(m);
    return isnan(f) ? -1 : ceil(f);
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
            return toPixelsF(m);
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

TypedSize TypedSize::add(TypedSize left, TypedSize right, TYPED_SIZE_TYPE outType, SizeMetrics m){
    return {outType, left.to(outType, m) + right.to(outType, m)};
}

TypedSize TypedSizeGetter_getSize(TypedSizeGetter *thiz){
    return thiz->data.size;
}

TypedSizeGetter TypedSizeGetter::from(TypedSize size){
    TypedSizeGetter getter;
    getter.data.size = size;
    getter.getSize = TypedSizeGetter_getSize;
    return getter;
}

TypedSize TypedSizeGetter::get(){
    return getSize(this);
}

TypedSize operator-(TypedSize size){
    size.value *= -1.0f;
    return size;
}