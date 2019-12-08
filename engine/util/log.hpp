//
// Created by Torben Thaysen on 03.02.2016.
//

#ifndef HEADER_UTIL_LOG_H
#define HEADER_UTIL_LOG_H

#ifdef PLATFORM_WINDOWS

#include<stdio.h>

#define LOGE(msg, ...) {printf(msg"\n", ##__VA_ARGS__); fflush(stdout);}
#define LOGW(msg, ...) {printf(msg"\n", ##__VA_ARGS__); fflush(stdout);}
#define LOGI(msg, ...) {printf(msg"\n", ##__VA_ARGS__); fflush(stdout);}
#define LOGD(msg, ...) {printf(msg"\n", ##__VA_ARGS__); fflush(stdout);}

#endif

#ifdef PLATFORM_ANDROID

#include <android/log.h>

#define TAG "engine"
#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, TAG,__VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG  , TAG,__VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO   , TAG,__VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN   , TAG,__VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR  , TAG,__VA_ARGS__)

#endif

#endif
