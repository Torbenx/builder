#include <jni.h>
#include "main.hpp"
#include <android/engine.hpp>

void engine_getRenderer(){
    engine_setRenderer(&createRenderer);
}

extern "C" {
    JNIEXPORT void JNICALL Java_com_tt_engine_MainActivity_nativeGetRenderer(JNIEnv *env, jobject thiz){
        engine_getRenderer();
    }
}
