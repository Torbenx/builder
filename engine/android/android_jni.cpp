#include "android/engine.hpp"
#include "util/log.hpp"
#include <android/asset_manager_jni.h>
#include <android/input.h>
#include <android/native_window_jni.h>
#include <errno.h>
#include <string.h>
#include <thread>
#include <unistd.h>

JavaVM* jvm;
jobject activity;
int engineWriteFd;
AAssetManager* assetManager;

jclass textClass;
jmethodID goFullscreenID, exitFullscreenID, finishID;
jmethodID textTextureConstructorID, drawTextID, textTextureReleaseID, createTextFormatID, getTextFormatWidthID, getTextFormatHeightID, getTextFormatLineCountID;

void android_engineGetJavaIds(JNIEnv* env) {
    jclass clazz = env->GetObjectClass(activity);
    goFullscreenID = env->GetMethodID(clazz, "goFullscreen", "()V");
    exitFullscreenID = env->GetMethodID(clazz, "exitFullscreen", "()V");
    finishID = env->GetMethodID(clazz, "finish", "()V");

    clazz = env->FindClass("com/tt/engine/Text");
    textClass = (jclass)env->NewGlobalRef(clazz);
    textTextureConstructorID = env->GetMethodID(clazz, "<init>", "(III)V");
    drawTextID = env->GetMethodID(clazz, "drawText", "(Landroid/text/StaticLayout;FFFFFFZ)[F");
    textTextureReleaseID = env->GetMethodID(clazz, "release", "()V");
    createTextFormatID = env->GetStaticMethodID(clazz, "createTextFormat", "(Ljava/lang/String;IF)Landroid/text/StaticLayout;");
    getTextFormatWidthID = env->GetStaticMethodID(clazz, "getTextFormatWidth", "(Landroid/text/StaticLayout;)F");
    getTextFormatHeightID = env->GetStaticMethodID(clazz, "getTextFormatHeight", "(Landroid/text/StaticLayout;)F");
    getTextFormatLineCountID = env->GetStaticMethodID(clazz, "getTextFormatLineCount", "(Landroid/text/StaticLayout;)I");
}

void android_engineThreadInit(JNIEnv* env, int readFd, int writeFd) {
    engineWriteFd = writeFd;
    android_engineGetJavaIds(env);
    android_initThread(readFd);
}

jobject android_createTextFormat(const char* text, int maxWidth, float textSize, float* width, float* height, int* lineCount) {
    JNIEnv* env;
    jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    jstring jText = env->NewStringUTF(text);
    jobject jFormat = env->CallStaticObjectMethod(textClass, createTextFormatID, jText, maxWidth, textSize);
    *width = env->CallStaticFloatMethod(textClass, getTextFormatWidthID, jFormat);
    *height = env->CallStaticFloatMethod(textClass, getTextFormatHeightID, jFormat);
    *lineCount = env->CallStaticIntMethod(textClass, getTextFormatLineCountID, jFormat);
    return env->NewGlobalRef(jFormat);
}

void android_releaseTextFormat(jobject jFormat) {
    JNIEnv* env;
    jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    env->DeleteGlobalRef(jFormat);
}

jobject android_textTextureConstructor(int glId, int width, int height) {
    JNIEnv* env;
    jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    jobject obj = env->NewObject(textClass, textTextureConstructorID, glId, width, height);
    return env->NewGlobalRef(obj);
}

void android_drawText(jobject thiz, jobject jFormat, float posX, float posY, float r, float g, float b, float a, bool clear, Mat4x4& matrix) {
    JNIEnv* env;
    jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    jfloatArray jMatrix = (jfloatArray)env->CallObjectMethod(thiz, drawTextID, jFormat, posX, posY, r, g, b, a, clear);
    env->GetFloatArrayRegion(jMatrix, 0, 16, matrix.matrix);
}

void android_textTextureRelease(jobject thiz) {
    JNIEnv* env;
    jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    env->CallVoidMethod(thiz, textTextureReleaseID);
    env->DeleteGlobalRef(thiz);
}

void android_goFullscreen() {
    JNIEnv* env;
    jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    env->CallVoidMethod(activity, goFullscreenID);
}

void android_exitFullscreen() {
    JNIEnv* env;
    jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    env->CallVoidMethod(activity, exitFullscreenID);
}

void android_sendBuf(int fd, void* buf, int size) {
    int ret = write(fd, buf, size);
    if (ret == -1) {
        log<Tag::Android>("write error: {}", strerror(errno));
    } else if (ret != size) {
        log<Tag::Android>("wrote wrong size: {}", ret);
    }
}

void android_sendCmd(int fd, cmd_t cmd) {
    android_sendBuf(fd, &cmd, sizeof(cmd_t));
}

void android_exit() {
    JNIEnv* env;
    jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    env->CallVoidMethod(activity, finishID);
}

extern "C" {

void android_onStart([[maybe_unused]] ANativeActivity* activity) {
    android_sendCmd(engineWriteFd, ENGINE_CMD_VISIBLE);
}

void android_onResume([[maybe_unused]] ANativeActivity* activity) {
    android_sendCmd(engineWriteFd, ENGINE_CMD_FOREGROUND);
}

void android_onPause([[maybe_unused]] ANativeActivity* activity) {
    android_sendCmd(engineWriteFd, ENGINE_CMD_BACKGROUND);
}

void android_onStop([[maybe_unused]] ANativeActivity* activity) {
    android_sendCmd(engineWriteFd, ENGINE_CMD_INVISIBLE);
}

void android_onDestroy([[maybe_unused]] ANativeActivity* activity) {
    android_sendCmd(engineWriteFd, ENGINE_CMD_DESTROY);
}

void android_onSurfaceCreated([[maybe_unused]] ANativeActivity* activity, ANativeWindow* window) {
    android_sendCmd(engineWriteFd, ENGINE_CMD_SURFACE_CREATE);
    struct SurfaceCmdData data = { window };
    android_sendBuf(engineWriteFd, &data, sizeof(struct SurfaceCmdData));
}

void android_onSurfaceChanged([[maybe_unused]] ANativeActivity* activity, ANativeWindow* window) {
    android_sendCmd(engineWriteFd, ENGINE_CMD_SURFACE_CHANGE);
    struct SurfaceChangeCmdData data = { window, ANativeWindow_getWidth(window), ANativeWindow_getHeight(window) };
    android_sendBuf(engineWriteFd, &data, sizeof(struct SurfaceChangeCmdData));
}

void android_onSurfaceDestroyed([[maybe_unused]] ANativeActivity* activity, ANativeWindow* window) {
    android_sendCmd(engineWriteFd, ENGINE_CMD_SURFACE_DESTROY);
    struct SurfaceCmdData data = { window };
    android_sendBuf(engineWriteFd, &data, sizeof(struct SurfaceCmdData));
}

void android_onInputQueueCreated([[maybe_unused]] ANativeActivity* activity, AInputQueue* inputQueue) {
    AInputQueue_attachLooper(inputQueue, looper, ENGINE_IDENT_INPUT, NULL, inputQueue);
}

void android_onInputQueueDestroyed([[maybe_unused]] ANativeActivity* activity, AInputQueue* inputQueue) {
    AInputQueue_detachLooper(inputQueue);
}

JNIEXPORT void JNICALL Java_com_tt_engine_MainActivity_nativeThreadInit([[maybe_unused]] JNIEnv* env, [[maybe_unused]] jobject thiz, jint readFd, jint writeFd) {
    android_engineThreadInit(env, readFd, writeFd);
}

JNIEXPORT void JNICALL Java_com_tt_engine_MainActivity_nativeThreadLoop([[maybe_unused]] JNIEnv* env, [[maybe_unused]] jobject thiz) {
    android_runThread();
}

JNIEXPORT void JNICALL Java_com_tt_engine_MainActivity_nativeCreate([[maybe_unused]] JNIEnv* env, [[maybe_unused]] jobject thiz) {
    android_sendCmd(engineWriteFd, ENGINE_CMD_CREATE);
}

}

void android_engineNativeActivityInit(ANativeActivity* nativeActivity) {

    nativeActivity->callbacks->onStart = android_onStart;
    nativeActivity->callbacks->onResume = android_onResume;
    nativeActivity->callbacks->onPause = android_onPause;
    nativeActivity->callbacks->onStop = android_onStop;
    nativeActivity->callbacks->onDestroy = android_onDestroy;
    nativeActivity->callbacks->onNativeWindowCreated = android_onSurfaceCreated;
    nativeActivity->callbacks->onNativeWindowResized = android_onSurfaceChanged;
    nativeActivity->callbacks->onNativeWindowDestroyed = android_onSurfaceDestroyed;
    nativeActivity->callbacks->onInputQueueCreated = android_onInputQueueCreated;
    nativeActivity->callbacks->onInputQueueDestroyed = android_onInputQueueDestroyed;

    activity = nativeActivity->clazz;
    assetManager = nativeActivity->assetManager;
    jvm = nativeActivity->vm;
}
