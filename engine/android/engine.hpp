#ifndef HEADER_ANDROID_ENGINE_H
#define HEADER_ANDROID_ENGINE_H

#include "../Renderer.hpp"
#include "../math/Mat4x4.hpp"
#include <android/asset_manager.h>
#include <jni.h>
#include <android/looper.h>
#include <android/native_window.h>

typedef int32_t cmd_t;

#define ENGINE_IDENT_CMD 1
#define ENGINE_IDENT_INPUT 2

#define ENGINE_CMD_CREATE 1
#define ENGINE_CMD_VISIBLE 2
#define ENGINE_CMD_FOREGROUND 3
#define ENGINE_CMD_BACKGROUND 4
#define ENGINE_CMD_INVISIBLE 5
#define ENGINE_CMD_DESTROY 6

#define ENGINE_CMD_SURFACE_CREATE 10
#define ENGINE_CMD_SURFACE_CHANGE 11
#define ENGINE_CMD_SURFACE_DESTROY 12

extern JavaVM *jvm;
extern AAssetManager *assetManager;
extern ALooper *looper;
extern jobject activity;

struct SurfaceCmdData{
    ANativeWindow *window;
};

struct SurfaceChangeCmdData{
    ANativeWindow *window;
    int width;
    int height;
};

void setRenderer(Renderer *renderer);

jobject android_createTextFormat(const char* text, int maxWidth, float textSize, float *width, float *height, int *lineCount);
void android_releaseTextFormat(jobject obj);

jobject android_textTextureConstructor(int glId, int width, int height);
void android_drawText(jobject thiz, jobject jFormat, float posX, float posY, float r, float g, float b, float a, bool clear, Mat4x4& matrix);
void android_textTextureRelease(jobject thiz);

void android_goFullscreen();
void android_exitFullscreen();

void android_initThread(int cmdFd);
void android_runThread();

#endif