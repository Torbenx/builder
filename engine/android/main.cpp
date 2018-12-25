#include <jni.h>
#include "Renderer.hpp"
#include "main.hpp"

Renderer *renderer;

void android_create(){
	renderer = createRenderer();
	renderer->create();
}

void android_change(int width, int height){
	renderer->change(width, height);
}

void android_drawFrame(){
	renderer->drawFrame();
}

extern "C" {
	JNIEXPORT void JNICALL Java_com_tt_engine_MainActivity_nativeCreate(JNIEnv env, jclass thiz){
		android_create();
	}
	
	JNIEXPORT void JNICALL Java_com_tt_engine_MainActivity_nativeChange(JNIEnv env, jclass thiz, jint width, jint height){
		android_change(width, height);
	}
	
	JNIEXPORT void JNICALL Java_com_tt_engine_MainActivity_nativeDrawFrame(JNIEnv env, jclass thiz){
		android_drawFrame();
	}
}