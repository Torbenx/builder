#ifndef HEADER_UI_TYPED_POINT_H
#define HEADER_UI_TYPED_POINT_H

#include "../math/Vec2.hpp"
#include "TypedSize.hpp"

enum TYPED_POINT_TYPE{
    TYPED_POINT_TOP_LOW_PIXELS,
    TYPED_POINT_BOTTOM_LOW_PIXELS,
    TYPED_POINT_GL,
    TYPED_POINT_WINDOWS_DIP,
    TYPED_POINT_ANDROID_DIP
};

struct TypedPoint{
    TYPED_POINT_TYPE type;
    Vec2 p;
    
    Vec2 toTopLowPixels(ScreenMetrics m);
    Vec2 toBottomLowPixels(ScreenMetrics m);
    Vec2 toAndroidPixels(ScreenMetrics m){return toTopLowPixels(m);}
    Vec2 toWindowsPixels(ScreenMetrics m){return toTopLowPixels(m);}
    Vec2 toWindowsDIP(ScreenMetrics m);
    Vec2 toAndroidDIP(ScreenMetrics m);
    Vec2 toGL(ScreenMetrics m);
    Vec2 to(TYPED_POINT_TYPE type, ScreenMetrics m);
    
    TYPED_SIZE_TYPE getSizeType();
    
    static TypedPoint topLowPixels(Vec2 p);
    static TypedPoint bottomLowPixels(Vec2 p);
    static TypedPoint androidPixels(Vec2 p){return topLowPixels(p);}
    static TypedPoint windowsPixels(Vec2 p){return topLowPixels(p);}
    static TypedPoint windowsDIP(Vec2 p);
    static TypedPoint androidDIP(Vec2 p);
    static TypedPoint gl(Vec2 p);
    
    static TYPED_SIZE_TYPE getSizeType(TYPED_POINT_TYPE type);
};

#endif