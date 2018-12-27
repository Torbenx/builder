#ifndef HEADER_ANDROID_ENGINE_H
#define HEADER_ANDROID_ENGINE_H

#include "../Renderer.hpp"
#include <android/asset_manager.h>

extern AAssetManager *assetManager;
void setRenderer(Renderer *renderer);

#endif