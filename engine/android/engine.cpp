#include <jni.h>
#include "../Renderer.hpp"
#include "engine.hpp"

#include <android/asset_manager_jni.h>

Renderer *renderer;
AAssetManager *assetManager;
JavaVM *jvm;
jobject activity;
jmethodID goFullscreenID, exitFullscreenID;
int rendererState = RENDERER_STATE_INVISIBLE;

void setRenderer(Renderer *r){
    renderer = r;
}

int Renderer::getState(){
    return rendererState;
}

void android_create(JNIEnv *env, jobject thiz){
    env->GetJavaVM(&jvm);
    activity = env->NewGlobalRef(thiz);
    jclass clazz = env->GetObjectClass(activity);
    goFullscreenID = env->GetMethodID(clazz, "goFullscreen", "()V");
    exitFullscreenID = env->GetMethodID(clazz, "exitFullscreen", "()V");
    renderer->create();
}

void android_visible(){
    rendererState = RENDERER_STATE_BACKGROUND;
    renderer->visible();
}

void android_foreground(){
    rendererState = RENDERER_STATE_FOREGROUND;
    renderer->foreground();
}

void android_background(){
    rendererState = RENDERER_STATE_BACKGROUND;
    renderer->background();
}

void android_invisible(){
    rendererState = RENDERER_STATE_INVISIBLE;
    renderer->invisible();
}

void android_destroy(JNIEnv *env){
    renderer->destroy();
    delete renderer;
    env->DeleteGlobalRef(activity);
}

void android_change(int width, int height){
    renderer->change(width, height);
}

void android_drawFrame(){
    renderer->drawFrame();
}

void android_touch(int ev, int pointerId, int x, int y){
    PointerEvent event;
    event.event = ev;
    event.pointerId = pointerId;
    event.x = x;
    event.y = y;
    renderer->pointer(event);
}

void android_goFullscreen(){
    JNIEnv *env;
    jvm->GetEnv((void**)&env ,JNI_VERSION_1_6);
    env->CallVoidMethod(activity, goFullscreenID);
}

void android_exitFullscreen(){
    JNIEnv *env;
    jvm->GetEnv((void**)&env ,JNI_VERSION_1_6);
    env->CallVoidMethod(activity, exitFullscreenID);
}

extern "C" {
    JNIEXPORT void JNICALL Java_com_tt_engine_MainActivity_nativeCreate(JNIEnv *env, jobject thiz, jobject jAssetManager){
        assetManager = AAssetManager_fromJava(env, jAssetManager);
        android_create(env, thiz);
    }
    
    JNIEXPORT void JNICALL Java_com_tt_engine_MainActivity_nativeStart(JNIEnv *env, jobject thiz){
        android_visible();
    }
    
    JNIEXPORT void JNICALL Java_com_tt_engine_MainActivity_nativeResume(JNIEnv *env, jobject thiz){
        android_foreground();
    }
    
    JNIEXPORT void JNICALL Java_com_tt_engine_MainActivity_nativePause(JNIEnv *env, jobject thiz){
        android_background();
    }
    
    JNIEXPORT void JNICALL Java_com_tt_engine_MainActivity_nativeStop(JNIEnv *env, jobject thiz){
        android_invisible();
    }
    
    JNIEXPORT void JNICALL Java_com_tt_engine_MainActivity_nativeDestroy(JNIEnv *env, jobject thiz){
        android_destroy(env);
        assetManager = nullptr;
    }
    
    JNIEXPORT void JNICALL Java_com_tt_engine_MainActivity_nativeChange(JNIEnv *env, jobject thiz, jint width, jint height){
        android_change(width, height);
    }
    
    JNIEXPORT void JNICALL Java_com_tt_engine_MainActivity_nativeDrawFrame(JNIEnv *env, jobject thiz){
        android_drawFrame();
    }
    
    JNIEXPORT void JNICALL Java_com_tt_engine_MainActivity_nativeTouch(JNIEnv *env, jobject thiz, jint ev, jint pointerId, jint x, jint y){
        android_touch(ev, pointerId, x, y);
    }
}