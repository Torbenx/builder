#ifndef HEADER_ANDROID_ENGINE_H
#define HEADER_ANDROID_ENGINE_H

#include "../Renderer.hpp"
#include <android/asset_manager.h>
#include <jni.h>

extern AAssetManager *assetManager;
void setRenderer(Renderer *renderer);
void android_goFullscreen();
void android_exitFullscreen();

#endif