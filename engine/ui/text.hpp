#ifndef HEADER_UI_TEXT_H
#define HEADER_UI_TEXT_H

#include "../graphic/Texture.hpp"
#include "../math/Vec4.hpp"
#include <string>

#ifdef PLATFORM_ANDROID
#define TEXT_TEXTURE_GLSL_SAMPLER "samplerExternalOES"
#define TEXT_TEXTURE_GLSL_HEADER "#extension GL_OES_EGL_image_external : require\n"
#else
#define TEXT_TEXTURE_GLSL_SAMPLER "sampler2D"
#define TEXT_TEXTURE_GLSL_HEADER
#endif

class TextTexture{
    public:
        void* internal;
        Texture texture;
        TextTexture();
        ~TextTexture();
        void init(int width, int height);
        void drawText(std::string text, int maxWidth, float textSize, Vec4 color);
};

#endif