//
// Created by Torben Thaysen on 03.02.2016.
//

#pragma once

#define VALIDATE_GL
#define VALIDATE_MATH
#define VALIDATE_MATH_MARGIN 0.00001f

#ifdef PLATFORM_WINDOWS

#include <stdio.h>
#include <cmath>

#define LOGV(msg, ...) {printf(msg"\n", ##__VA_ARGS__); fflush(stdout);}
#define LOGD(msg, ...) {printf(msg"\n", ##__VA_ARGS__); fflush(stdout);}
#define LOGI(msg, ...) {printf(msg"\n", ##__VA_ARGS__); fflush(stdout);}
#define LOGW(msg, ...) {printf(msg"\n", ##__VA_ARGS__); fflush(stdout);}
#define LOGE(msg, ...) {printf(msg"\n", ##__VA_ARGS__); fflush(stdout);}

#endif

#ifdef PLATFORM_ANDROID

#include <android/log.h>

#define TAG "engine"
#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG  , TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO   , TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN   , TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR  , TAG, __VA_ARGS__)

#endif

#ifdef VALIDATE_GL
#define GL_VERIFY(cond, ...) { if(!(cond)) LOGW("GL_VERIFY failed %s:%d: %s(): " #cond "; " __VA_ARGS__, __FILE__, __LINE__, __func__); }
#define GL_EXPECT(lhs, rhs, ...) { GLenum l = (lhs); GLenum r = (rhs); if(l != r) LOGW("GL_EXPECT failed: %s:%d: %s(): " #lhs " {0x%x} == " #rhs " {0x%x}; " __VA_ARGS__, __FILE__, __LINE__, __func__, l, r); }
#define GL_GET_EXPECT(pname, rhs, ...) { GLint l; glGetIntegerv(pname, &l); GLenum r = (rhs); if(l != r) LOGW("GL_GET_EXPECT failed: %s:%d: %s(): " #pname " {0x%x} == " #rhs " {0x%x}; " __VA_ARGS__, __FILE__, __LINE__, __func__, l, r); }
#define GL_VERIFY_NOT_REACHED(...) GL_VERIFY(false, __VA_ARGS__)
#else
#define GL_VERIFY(cond, ...)
#define GL_EXPECT(lhs, rhs, ...)
#define GL_GET_EXPECT(pname, rhs, ...)
#define GL_VERIFY_NOT_REACHED(...)
#endif

#ifdef VALIDATE_MATH
#define MATH_EXPECT(lhs, rhs, ...) { float l = (lhs); float r = (rhs); if(fabs(l - r) > VALIDATE_MATH_MARGIN) LOGW("MATH_EXPECT failed: %s:%d: %s(): " #lhs " {%f} == " #rhs " {%f}; " __VA_ARGS__, __FILE__, __LINE__, __func__, l, r); }
#define MATH_EXPECT_NORMALIZED(v, ...) { float len = v.length(); if(fabs(len - 1.0f) > VALIDATE_MATH_MARGIN) LOGW("MATH_EXPECT_NORMALIZED failed: %s:%d: %s(): " #v " {%f}; " __VA_ARGS__, __FILE__, __LINE__, __func__, len); }
#else
#define MATH_EXPECT(lhs, rhs, ...)
#define MATH_EXPECT_NORMALIZED(v, ...)
#endif
