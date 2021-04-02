#include "ui/TypedPoint.hpp"
#include <math.h>
#include <cassert>

Vec2 TypedPoint::toTopLowPixels(ScreenMetrics m){
    switch(type){
        case TYPED_POINT_TOP_LOW_PIXELS:
            return p;
        case TYPED_POINT_BOTTOM_LOW_PIXELS:
            return Vec2(p.x, m.y.size - p.y);
        case TYPED_POINT_GL:
            return (p + Vec2(-1.0f, 1.0f)) / 2.0f * Vec2(m.x.size, -m.y.size);
        case TYPED_POINT_WINDOWS_DIP:
            return p * Vec2(m.x.dpi, m.y.dpi) / 96.0f;
        case TYPED_POINT_ANDROID_DIP:
            return p * Vec2(m.x.dpi, m.y.dpi) / 160.0f;
        default:
            throw 0;
    }
}

Vec2 TypedPoint::toBottomLowPixels(ScreenMetrics m){
    switch(type){
        case TYPED_POINT_TOP_LOW_PIXELS:
            return Vec2(p.x, m.y.size - p.y);
        case TYPED_POINT_BOTTOM_LOW_PIXELS:
            return p;
        case TYPED_POINT_GL:
            return (p + Vec2(-1.0f)) / 2.0f * Vec2(m.x.size, m.y.size);
        case TYPED_POINT_WINDOWS_DIP:
            return Vec2(p.x, m.y.size - p.y) * Vec2(m.x.dpi, m.y.dpi) / 96.0f;
        case TYPED_POINT_ANDROID_DIP:
            return Vec2(p.x, m.y.size - p.y) * Vec2(m.x.dpi, m.y.dpi) / 160.0f;
        default:
            throw 0;
    }
}

Vec2 TypedPoint::toGL(ScreenMetrics m){
    switch(type){
        case TYPED_POINT_TOP_LOW_PIXELS:
            return p * 2.0f * Vec2(1.0f/m.x.size, -1.0f/m.y.size) + Vec2(-1.0f, 1.0f);
        case TYPED_POINT_BOTTOM_LOW_PIXELS:
            return p * 2.0f * Vec2(1.0f/m.x.size, 1.0f/m.y.size) + Vec2(-1.0f);
        case TYPED_POINT_GL:
            return p;
        case TYPED_POINT_WINDOWS_DIP:
            return p * Vec2(m.x.dpi, m.y.dpi) / 96.0f * 2.0f * Vec2(1.0f/m.x.size, -1.0f/m.y.size) + Vec2(-1.0f, 1.0f);
        case TYPED_POINT_ANDROID_DIP:
            return p * Vec2(m.x.dpi, m.y.dpi) / 160.0f * 2.0f * Vec2(1.0f/m.x.size, -1.0f/m.y.size) + Vec2(-1.0f, 1.0f);
        default:
            throw 0;
    }
}

Vec2 TypedPoint::toWindowsDIP(ScreenMetrics m){
    switch(type){
        case TYPED_POINT_TOP_LOW_PIXELS:
            return p * 96.0f * Vec2(1.0f/m.x.dpi, 1.0f/m.y.dpi);
        case TYPED_POINT_BOTTOM_LOW_PIXELS:
            return Vec2(p.x, m.y.size - p.y) * 96.0f * Vec2(1.0f/m.x.dpi, 1.0f/m.y.dpi);
        case TYPED_POINT_GL:
            return (p + Vec2(-1.0f, 1.0f)) / 2.0f * Vec2(m.x.size, -m.y.size) * 96.0f * Vec2(1.0f/m.x.dpi, 1.0f/m.y.dpi);
        case TYPED_POINT_WINDOWS_DIP:
            return p;
        case TYPED_POINT_ANDROID_DIP:
            return p / 160.0f * 96.0f;
        default:
            throw 0;
    }
}

Vec2 TypedPoint::toAndroidDIP(ScreenMetrics m){
    switch(type){
        case TYPED_POINT_TOP_LOW_PIXELS:
            return p * 160.0f * Vec2(1.0f/m.x.dpi, 1.0f/m.y.dpi);
        case TYPED_POINT_BOTTOM_LOW_PIXELS:
            return Vec2(p.x, m.y.size - p.y) * 160.0f * Vec2(1.0f/m.x.dpi, 1.0f/m.y.dpi);
        case TYPED_POINT_GL:
            return (p + Vec2(-1.0f, 1.0f)) / 2.0f * Vec2(m.x.size, -m.y.size) * 160.0f * Vec2(1.0f/m.x.dpi, 1.0f/m.y.dpi);
        case TYPED_POINT_WINDOWS_DIP:
            return p / 96.0f * 160.0f;
        case TYPED_POINT_ANDROID_DIP:
            return p;
        default:
            throw 0;
    }
}

TYPED_SIZE_TYPE TypedPoint::getSizeType(){
    return TypedPoint::getSizeType(type);
}

TypedPoint TypedPoint::topLowPixels(Vec2 p){
    return {TYPED_POINT_TOP_LOW_PIXELS, p};
}

TypedPoint TypedPoint::bottomLowPixels(Vec2 p){
    return {TYPED_POINT_BOTTOM_LOW_PIXELS, p};
}

TypedPoint TypedPoint::windowsDIP(Vec2 p){
    return {TYPED_POINT_WINDOWS_DIP, p};
}

TypedPoint TypedPoint::androidDIP(Vec2 p){
    return {TYPED_POINT_ANDROID_DIP, p};
}

TypedPoint TypedPoint::gl(Vec2 p){
    return {TYPED_POINT_GL, p};
}

TYPED_SIZE_TYPE TypedPoint::getSizeType(TYPED_POINT_TYPE type){
    switch(type){
        case TYPED_POINT_TOP_LOW_PIXELS:
            return TYPED_SIZE_PIXELS;
        case TYPED_POINT_BOTTOM_LOW_PIXELS:
            return TYPED_SIZE_PIXELS;
        case TYPED_POINT_GL:
            return TYPED_SIZE_GL;
        case TYPED_POINT_WINDOWS_DIP:
            return TYPED_SIZE_WINDOWS_DIP;
        case TYPED_POINT_ANDROID_DIP:
            return TYPED_SIZE_ANDROID_DIP;
        default:
            throw 0;
    }
}
