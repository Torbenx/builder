#include <jni.h>
#include "../Renderer.hpp"
#include "engine.hpp"

#include <android/asset_manager_jni.h>

Renderer *renderer;
AAssetManager *assetManager;

void setRenderer(Renderer *r){
    renderer = r;
}

void android_create(){
    
    renderer->create();
}

void android_change(int width, int height){
    renderer->change(width, height);
}

void android_drawFrame(){
    renderer->drawFrame();
}

void android_destroy(){
    renderer->destroy();
    delete renderer;
}

extern "C" {
    JNIEXPORT void JNICALL Java_com_tt_engine_MainActivity_nativeCreate(JNIEnv *env, jobject thiz, jobject jAssetManager){
        assetManager = AAssetManager_fromJava(env, jAssetManager);
        android_create();
    }
    
    JNIEXPORT void JNICALL Java_com_tt_engine_MainActivity_nativeChange(JNIEnv *env, jobject thiz, jint width, jint height){
        android_change(width, height);
    }
    
    JNIEXPORT void JNICALL Java_com_tt_engine_MainActivity_nativeDrawFrame(JNIEnv *env, jobject thiz){
        android_drawFrame();
    }
    
    JNIEXPORT void JNICALL Java_com_tt_engine_MainActivity_nativeDestroy(JNIEnv *env, jobject thiz){
        android_destroy();
        assetManager = nullptr;
    }
}