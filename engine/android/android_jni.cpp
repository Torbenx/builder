#include "engine.hpp"
#include "../util/log.hpp"
#include <android/asset_manager_jni.h>
#include <android/native_window_jni.h>
#include <android/input.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

JavaVM *jvm;
jobject activity;
AAssetManager *assetManager;

jmethodID goFullscreenID, exitFullscreenID, InputQueue_getNativePtrID;

void android_engineThreadInit(JNIEnv *env, jobject thiz, jobject jAssetManager, int cmdFd){
    env->GetJavaVM(&jvm);
    activity = env->NewGlobalRef(thiz);
    jclass clazz = env->GetObjectClass(activity);
    goFullscreenID = env->GetMethodID(clazz, "goFullscreen", "()V");
    exitFullscreenID = env->GetMethodID(clazz, "exitFullscreen", "()V");
    clazz = env->FindClass("android/view/InputQueue");
    InputQueue_getNativePtrID = env->GetMethodID(clazz, "getNativePtr", "()J");
    assetManager = AAssetManager_fromJava(env, jAssetManager);
    android_initThread(cmdFd);
}

void android_goFullscreen(){
    JNIEnv *env;
    jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    env->CallVoidMethod(activity, goFullscreenID);
}

void android_exitFullscreen(){
    JNIEnv *env;
    jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    env->CallVoidMethod(activity, exitFullscreenID);
}

void android_sendBuf(int fd, void *buf, int size){
    int ret = write(fd, buf, size);
    if(ret == -1){
        LOGW("write error: %s", strerror(errno));
    }else if(ret != size){
        LOGW("wrote wrong size: %d",ret);
    }
}

void android_sendCmd(int fd, cmd_t cmd){
    android_sendBuf(fd, &cmd, sizeof(cmd_t));
}

extern "C" {
    JNIEXPORT void JNICALL Java_com_tt_engine_MainActivity_nativeThreadInit(JNIEnv *env, jobject thiz, jobject jAssetManager, jint cmdFd){
        android_engineThreadInit(env, thiz, jAssetManager, cmdFd);
    }
    
    JNIEXPORT void JNICALL Java_com_tt_engine_MainActivity_nativeThreadLoop(JNIEnv *env, jobject thiz){
        android_runThread();
    }
    
    JNIEXPORT void JNICALL Java_com_tt_engine_MainActivity_nativeCreate(JNIEnv *env, jobject thiz, jint fd){
        android_sendCmd(fd, ENGINE_CMD_CREATE);
    }
    
    JNIEXPORT void JNICALL Java_com_tt_engine_MainActivity_nativeStart(JNIEnv *env, jobject thiz, jint fd){
        android_sendCmd(fd, ENGINE_CMD_VISIBLE);
    }
    
    JNIEXPORT void JNICALL Java_com_tt_engine_MainActivity_nativeResume(JNIEnv *env, jobject thiz, jint fd){
        android_sendCmd(fd, ENGINE_CMD_FOREGROUND);
    }
    
    JNIEXPORT void JNICALL Java_com_tt_engine_MainActivity_nativePause(JNIEnv *env, jobject thiz, jint fd){
        android_sendCmd(fd, ENGINE_CMD_BACKGROUND);
    }
    
    JNIEXPORT void JNICALL Java_com_tt_engine_MainActivity_nativeStop(JNIEnv *env, jobject thiz, jint fd){
        android_sendCmd(fd, ENGINE_CMD_INVISIBLE);
    }
    
    JNIEXPORT void JNICALL Java_com_tt_engine_MainActivity_nativeDestroy(JNIEnv *env, jobject thiz, jint fd){
        android_sendCmd(fd, ENGINE_CMD_DESTROY);
    }
    
    JNIEXPORT void JNICALL Java_com_tt_engine_MainActivity_nativeSurfaceCreate(JNIEnv *env, jobject thiz, jint fd, jobject jSurface){
        android_sendCmd(fd, ENGINE_CMD_SURFACE_CREATE);
        ANativeWindow *cmdWindow = ANativeWindow_fromSurface(env, jSurface);
        struct SurfaceCmdData data = {cmdWindow};
        android_sendBuf(fd, &data, sizeof(struct SurfaceCmdData));
    }
    
    JNIEXPORT void JNICALL Java_com_tt_engine_MainActivity_nativeSurfaceChange(JNIEnv *env, jobject thiz, jint fd, jobject jSurface, jint width, jint height){
       android_sendCmd(fd, ENGINE_CMD_SURFACE_CHANGE);
        ANativeWindow *cmdWindow = ANativeWindow_fromSurface(env, jSurface);
        struct SurfaceChangeCmdData data = {cmdWindow, width, height};
        android_sendBuf(fd, &data, sizeof(struct SurfaceChangeCmdData));
    }
    
    JNIEXPORT void JNICALL Java_com_tt_engine_MainActivity_nativeSurfaceDestroy(JNIEnv *env, jobject thiz, jint fd, jobject jSurface){
        android_sendCmd(fd, ENGINE_CMD_SURFACE_DESTROY);
        ANativeWindow *cmdWindow = ANativeWindow_fromSurface(env, jSurface);
        struct SurfaceCmdData data = {cmdWindow};
        android_sendBuf(fd, &data, sizeof(struct SurfaceCmdData));
    }
    
    JNIEXPORT void JNICALL Java_com_tt_engine_MainActivity_nativeCreateInputQueue(JNIEnv *env, jobject thiz, jobject jInputQueue){
        //since api 19
        AInputQueue *inputQueue = (AInputQueue*)env->CallLongMethod(jInputQueue, InputQueue_getNativePtrID);
        AInputQueue_attachLooper(inputQueue, looper, ENGINE_IDENT_INPUT, NULL, inputQueue);
    }
    
    JNIEXPORT void JNICALL Java_com_tt_engine_MainActivity_nativeDestroyInputQueue(JNIEnv *env, jobject thiz, jobject jInputQueue){
        //since api 19
        AInputQueue *inputQueue = (AInputQueue*)env->CallLongMethod(jInputQueue, InputQueue_getNativePtrID);
        AInputQueue_detachLooper(inputQueue);
    }
}