#pragma once

#include "GL/Texture.hpp"
#include "math/Vec4.hpp"
#include "util/target.hpp"
#include <string>

#ifdef PLATFORM_ANDROID
#define TEXT_TEXTURE_GLSL_SAMPLER "samplerExternalOES"
#define TEXT_TEXTURE_GLSL_EXTENSION "GL_OES_EGL_image_external_essl3"
#else
#define TEXT_TEXTURE_GLSL_SAMPLER "sampler2D"
#endif

class TextFormat {
public:
    void* internal;
    TextFormat(std::string, int maxWidth, float textSize);
    ~TextFormat();
    float getWidth();
    float getHeight();
    int getLineCount();

private:
    TextFormat(const TextFormat& other) {};
};

class TextTexture : public Texture {
public:
    void* internal;
    TextTexture(int width, int height);
    ~TextTexture();
    void drawText(const TextFormat& text, Vec2 position, Vec4 color, bool clear);

private:
    TextTexture(const TextTexture& other);
};
