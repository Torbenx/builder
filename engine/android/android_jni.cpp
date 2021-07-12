#include "android/engine.hpp"
#include "util/log.hpp"
#include <android/asset_manager_jni.h>
#include <android/native_window_jni.h>
#include <android/input.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

JavaVM *jvm;
jobject activity;
AAssetManager *assetManager;

jclass textClass;
jmethodID goFullscreenID, exitFullscreenID, InputQueue_getNativePtrID, finishID;
jmethodID textTextureConstructorID, drawTextID, textTextureReleaseID, createTextFormatID, getTextFormatWidthID, getTextFormatHeightID, getTextFormatLineCountID;

void android_engineThreadInit(JNIEnv *env, jobject thiz, jobject jAssetManager, int cmdFd){
    env->GetJavaVM(&jvm);

    activity = env->NewGlobalRef(thiz);

    jclass clazz = env->GetObjectClass(activity);
    goFullscreenID = env->GetMethodID(clazz, "goFullscreen", "()V");
    exitFullscreenID = env->GetMethodID(clazz, "exitFullscreen", "()V");
    finishID = env->GetMethodID(clazz, "finish", "()V");

    clazz = env->FindClass("android/view/InputQueue");
    InputQueue_getNativePtrID = env->GetMethodID(clazz, "getNativePtr", "()J");

    clazz = env->FindClass("com/tt/engine/Text");
    textClass = (jclass)env->NewGlobalRef(clazz);
    textTextureConstructorID = env->GetMethodID(clazz, "<init>", "(III)V");
    drawTextID = env->GetMethodID(clazz, "drawText", "(Landroid/text/StaticLayout;FFFFFFZ)[F");
    textTextureReleaseID = env->GetMethodID(clazz, "release", "()V");
    createTextFormatID = env->GetStaticMethodID(clazz, "createTextFormat", "(Ljava/lang/String;IF)Landroid/text/StaticLayout;");
    getTextFormatWidthID = env->GetStaticMethodID(clazz, "getTextFormatWidth", "(Landroid/text/StaticLayout;)F");
    getTextFormatHeightID = env->GetStaticMethodID(clazz, "getTextFormatHeight", "(Landroid/text/StaticLayout;)F");
    getTextFormatLineCountID = env->GetStaticMethodID(clazz, "getTextFormatLineCount", "(Landroid/text/StaticLayout;)I");

    assetManager = AAssetManager_fromJava(env, jAssetManager);

    android_initThread(cmdFd);
}

jobject android_createTextFormat(const char *text, int maxWidth, float textSize, float *width, float *height, int *lineCount){
    JNIEnv *env;
    jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    jstring jText = env->NewStringUTF(text);
    jobject jFormat = env->CallStaticObjectMethod(textClass, createTextFormatID, jText, maxWidth, textSize);
    *width = env->CallStaticFloatMethod(textClass, getTextFormatWidthID, jFormat);
    *height = env->CallStaticFloatMethod(textClass, getTextFormatHeightID, jFormat);
    *lineCount = env->CallStaticIntMethod(textClass, getTextFormatLineCountID, jFormat);
    return env->NewGlobalRef(jFormat);
}

void android_releaseTextFormat(jobject jFormat){
    JNIEnv *env;
    jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    env->DeleteGlobalRef(jFormat);
}

jobject android_textTextureConstructor(int glId, int width, int height){
    JNIEnv *env;
    jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    jobject obj = env->NewObject(textClass, textTextureConstructorID, glId, width, height);
    return env->NewGlobalRef(obj);
}

void android_drawText(jobject thiz, jobject jFormat, float posX, float posY, float r, float g, float b, float a, bool clear, Mat4x4& matrix){
    JNIEnv *env;
    jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    jfloatArray jMatrix = (jfloatArray)env->CallObjectMethod(thiz, drawTextID, jFormat, posX, posY, r, g, b, a, clear);
    env->GetFloatArrayRegion(jMatrix, 0, 16, matrix.matrix);
}

void android_textTextureRelease(jobject thiz){
    JNIEnv *env;
    jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    env->CallVoidMethod(thiz, textTextureReleaseID);
    env->DeleteGlobalRef(thiz);
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
        log<Tag::Android>("write error: {}", strerror(errno));
    }else if(ret != size){
        log<Tag::Android>("wrote wrong size: {}",ret);
    }
}

void android_sendCmd(int fd, cmd_t cmd){
    android_sendBuf(fd, &cmd, sizeof(cmd_t));
}

void android_exit(){
    JNIEnv *env;
    jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    env->CallVoidMethod(activity, finishID);
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
