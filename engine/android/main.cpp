#include "main.hpp"
#include "android/engine.hpp"
#include <jni.h>

void engine_getRenderer() {
    struct CreateFunctions functions;
    #define __FOREACH_G_API(api) functions.createRenderer_##api = createRenderer<api>;
    ENUMERATE_G_APIS(__FOREACH_G_API)
    #undef __FOREACH_G_API
    engine_setRenderer(functions);
}

extern "C" {

JNIEXPORT void JNICALL Java_com_tt_engine_MainActivity_nativeGetRenderer(JNIEnv* env, jobject thiz) {
    engine_getRenderer();
}

void ANativeActivity_onCreate(ANativeActivity* activity, [[maybe_unused]] void* savedState, [[maybe_unused]] size_t savedStateSize) {
    engine_getRenderer();
    android_engineNativeActivityInit(activity);
}

}
