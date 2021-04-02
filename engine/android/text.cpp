#include "util/text.hpp"
#include "android/engine.hpp"
#include <jni.h>

typedef struct{
    jobject format;
    float width;
    float height;
    int lineCount;
} AndroidTextFormatImpl;

typedef struct{
    jobject obj;
    Mat4x4 matrix;
} SurfaceTextureTextImpl;

TextFormat::TextFormat(std::string text, int maxWidth, float textSize){
    AndroidTextFormatImpl *format = new AndroidTextFormatImpl();
    format->format = android_createTextFormat(text.c_str(), maxWidth, textSize, &format->width, &format->height, &format->lineCount);
    internal = format;
}

float TextFormat::getWidth(){
    AndroidTextFormatImpl *format = (AndroidTextFormatImpl*)internal;
    return format->width;
}

float TextFormat::getHeight(){
    AndroidTextFormatImpl *format = (AndroidTextFormatImpl*)internal;
    return format->height;
}

int TextFormat::getLineCount(){
    AndroidTextFormatImpl *format = (AndroidTextFormatImpl*)internal;
    return format->lineCount;
}

TextFormat::~TextFormat(){
    AndroidTextFormatImpl *format = (AndroidTextFormatImpl*)internal;
    android_releaseTextFormat(format->format);
    delete format;
}

TextTexture::TextTexture(int width, int height) : Texture(false, width, height, GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_BINDING_EXTERNAL_OES){
    SurfaceTextureTextImpl *impl = new SurfaceTextureTextImpl();
    internal = impl;
    impl->obj = android_textTextureConstructor(glId, width, height);
}

void TextTexture::drawText(const TextFormat& format, Vec2 position, Vec4 color, bool clear){
    AndroidTextFormatImpl *formatImpl = (AndroidTextFormatImpl*)format.internal;
    SurfaceTextureTextImpl *impl = (SurfaceTextureTextImpl*)internal;
    android_drawText(impl->obj, formatImpl->format, position.x, position.y, color.x, color.y, color.z, color.w, clear, impl->matrix);
}

TextTexture::~TextTexture(){
    SurfaceTextureTextImpl *impl = (SurfaceTextureTextImpl*)internal;
    android_textTextureRelease(impl->obj);
    delete impl;
}
