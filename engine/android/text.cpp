#include "../ui/text.hpp"
#include "engine.hpp"
#include <jni.h>

typedef struct{
    jobject obj;
    Mat4x4 matrix;
} SurfaceTextureTextImpl;

TextTexture::TextTexture() : texture(GL_TEXTURE_EXTERNAL_OES){
    internal = new SurfaceTextureTextImpl();
}

void TextTexture::init(int width, int height){
    SurfaceTextureTextImpl *impl = (SurfaceTextureTextImpl*)internal;
    texture.init();
    impl->obj = android_textConstructor(texture.glId, width, height);
    texture.width = width;
    texture.height = height;
}

void TextTexture::drawText(std::string text, int maxWidth, float textSize, Vec4 color){
    SurfaceTextureTextImpl *impl = (SurfaceTextureTextImpl*)internal;
    android_drawText(impl->obj, text.c_str(), maxWidth, textSize, color.x, color.y, color.z, color.w, impl->matrix);
}

TextTexture::~TextTexture(){
    SurfaceTextureTextImpl *impl = (SurfaceTextureTextImpl*)internal;
    android_textRelease(impl->obj);
    delete impl;
}