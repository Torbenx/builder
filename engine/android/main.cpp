#include <jni.h>
#include "main.hpp"
#include "android/engine.hpp"

void init(){
    setRenderer(createRenderer());
}

extern "C" {
    JNIEXPORT void JNICALL Java_com_tt_engine_MainActivity_nativeInit(JNIEnv *env, jobject thiz){
        init();
    }
}