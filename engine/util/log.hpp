//
// Created by Torben Thaysen on 03.02.2016.
//

#pragma once

#include "util/target.hpp"
#include <type_traits>
#include <fmt/core.h>

#define VALIDATE_GL
#define VALIDATE_MATH
#define VALIDATE_MATH_MARGIN 0.00001f

#ifdef PLATFORM_WINDOWS

#include <iostream>

static void log_print(const std::string& tag, const std::string& msg) {
    std::cout << '[' << tag << "] " << msg << '\n';
}

#include <stdio.h>
#include <cmath>

#define LOGV(msg, ...) {fprintf(stdout, msg"\n", ##__VA_ARGS__); fflush(stdout);}
#define LOGD(msg, ...) {fprintf(stdout, msg"\n", ##__VA_ARGS__); fflush(stdout);}
#define LOGI(msg, ...) {fprintf(stdout, msg"\n", ##__VA_ARGS__); fflush(stdout);}
#define LOGW(msg, ...) {fprintf(stdout, msg"\n", ##__VA_ARGS__); fflush(stdout);}
#define LOGE(msg, ...) {fprintf(stdout, msg"\n", ##__VA_ARGS__); fflush(stdout);}

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

template<typename T>
concept LogTag = std::is_same_v<decltype(T::tag), std::string>;

struct DefaultLogTag {
    static inline std::string tag = "default";
};

template<LogTag tag = DefaultLogTag, typename... Args>
static void log(fmt::format_string<Args...> str, Args... args) {
    std::string msg = fmt::format(str, args...);
    log_print(tag::tag, msg);
}

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
#define MATH_VERIFY(cond, ...) { if(!(cond)) LOGW("MATH_VERIFY failed %s:%d: %s(): " #cond "; " __VA_ARGS__, __FILE__, __LINE__, __func__); }
#define MATH_EXPECT(lhs, rhs, ...) { float l = (lhs); float r = (rhs); if(fabs(l - r) > VALIDATE_MATH_MARGIN) LOGW("MATH_EXPECT failed: %s:%d: %s(): " #lhs " {%f} == " #rhs " {%f}; " __VA_ARGS__, __FILE__, __LINE__, __func__, l, r); }
#define MATH_EXPECT_NORMALIZED(v, ...) { float len = v.length(); if(fabs(len - 1.0f) > VALIDATE_MATH_MARGIN) LOGW("MATH_EXPECT_NORMALIZED failed: %s:%d: %s(): " #v " {%f}; " __VA_ARGS__, __FILE__, __LINE__, __func__, len); }
#define MATH_VARIFY_NOT_REACHED(...) MATH_VERIFY(false, __VA_ARGS__)
#else
#define MATH_VARIFY(cond, ...)
#define MATH_VARIFY_NOT_REACHED(...)
#define MATH_EXPECT(lhs, rhs, ...)
#define MATH_EXPECT_NORMALIZED(v, ...)
#endif
