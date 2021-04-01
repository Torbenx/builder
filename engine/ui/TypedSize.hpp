#ifndef HEADER_UI_TYPED_SIZE_H
#define HEADER_UI_TYPED_SIZE_H

struct SizeMetrics{
    int size;
    int dpi;
};

struct ScreenMetrics{
    SizeMetrics x;
    SizeMetrics y;

    int width() { return x.size; }
    int height() {return y.size; }
};

enum TYPED_SIZE_TYPE{
    TYPED_SIZE_PIXELS,
    TYPED_SIZE_GL,
    TYPED_SIZE_ANDROID_DIP,
    TYPED_SIZE_WINDOWS_DIP
};

struct TypedSizeXY;
struct LazyTypedSizeXY;

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

    TypedSizeXY asXSize();
    TypedSizeXY asYSize();

    static TypedSize pixels(float f);
    static TypedSize gl(float f);
    static TypedSize androidDIP(float f);
    static TypedSize windowsDIP(float f);

    TypedSize& operator*=(float f);
};

struct TypedSizeXY{
    bool isX;
    TypedSize size;

    float toPixels(ScreenMetrics metrics);
    float toGL(ScreenMetrics metrics);
    float toAndroidDIP(ScreenMetrics metrics);
    float toWindowsDIP(ScreenMetrics metrics);
    float to(TYPED_SIZE_TYPE type, ScreenMetrics m);

    LazyTypedSizeXY lazy();
};

struct LazyTypedSizeXY{
    union{
        void *ptr;
        TypedSizeXY size;
    } data;
    TypedSizeXY(*getSize)(LazyTypedSizeXY *thiz);
    TypedSizeXY get();
};

TypedSize operator-(TypedSize size);
TypedSize operator*(float left, TypedSize right);
TypedSize operator*(TypedSize left, float right);

#endif
