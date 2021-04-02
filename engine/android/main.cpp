#include "main.hpp"
#include <jni.h>
#include <android/engine.hpp>

void engine_getRenderer(){
    engine_setRenderer(&createRenderer);
}

extern "C" {
    JNIEXPORT void JNICALL Java_com_tt_engine_MainActivity_nativeGetRenderer(JNIEnv *env, jobject thiz){
        engine_getRenderer();
    }
}
