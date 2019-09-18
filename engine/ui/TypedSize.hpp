#ifndef HEADER_UI_TYPED_SIZE_H
#define HEADER_UI_TYPED_SIZE_H

struct SizeMetrics{
    int size;
    int dpi;
};

struct ScreenMetrics{
    SizeMetrics x;
    SizeMetrics y;
};

enum TYPED_SIZE_TYPE{
    TYPED_SIZE_PIXELS,
    TYPED_SIZE_GL,
    TYPED_SIZE_ANDROID_DIP,
    TYPED_SIZE_WINDOWS_DIP
};

struct TypedSize{
    static TypedSize zero;
    static TypedSize nan;
    
    TYPED_SIZE_TYPE type;
    float value;
    
    float toPixels(SizeMetrics metrics);
    float toGL(SizeMetrics metrics);
    float toAndroidDIP(SizeMetrics metrics);
    float toWindowsDIP(SizeMetrics metrics);
    float to(TYPED_SIZE_TYPE type, SizeMetrics m);
    
    static TypedSize pixels(float f);
    static TypedSize gl(float f);
    static TypedSize androidDIP(float f);
    static TypedSize windowsDIP(float f);
    
    static TypedSize add(TypedSize left, TypedSize right, TYPED_SIZE_TYPE outType, SizeMetrics m);
};

struct TypedSizeGetter{
    void *data;
    TypedSize(*getSize)(TypedSizeGetter *thiz);
    TypedSize get();
};

TypedSize operator-(TypedSize size);

#endif