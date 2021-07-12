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

#endif

#ifdef PLATFORM_ANDROID

#include <android/log.h>

static void log_print(const std::string& tag, const std::string& msg) {
    __android_log_write(ANDROID_LOG_DEBUG, tag.c_str(), msg.c_str());
}

#endif

template<typename T>
concept LogTag = std::is_convertible_v<decltype(T::tag), const char*>;

namespace Tag{
struct Default {
    static constexpr char tag[] = "default";
};
}

template<LogTag tag = Tag::Default, typename... Args>
static void log(fmt::format_string<Args...> str, Args&&... args) {
    std::string msg = fmt::format(str, std::forward<Args>(args)...);
    log_print(tag::tag, msg);
}

#ifdef VALIDATE_GL
#define GL_VERIFY(cond, ...) { if(!(cond)) log("GL_VERIFY failed {}:{}: {}(): " #cond "; " __VA_ARGS__, __FILE__, __LINE__, __func__); }
#define GL_EXPECT(lhs, rhs, ...) { GLenum l = (lhs); GLenum r = (rhs); if(l != r) log("GL_EXPECT failed: {}:{}: {}(): " #lhs " {{{:#x}}} == " #rhs " {{{:#x}}}; " __VA_ARGS__, __FILE__, __LINE__, __func__, l, r); }
#define GL_GET_EXPECT(pname, rhs, ...) { GLint l; glGetIntegerv(pname, &l); GLenum r = (rhs); if(l != r) log("GL_GET_EXPECT failed: {}:{}: {}(): " #pname " {{{:#x}}} == " #rhs " {{{:#x}}}; " __VA_ARGS__, __FILE__, __LINE__, __func__, l, r); }
#define GL_VERIFY_NOT_REACHED(...) GL_VERIFY(false, __VA_ARGS__)
#else
#define GL_VERIFY(cond, ...)
#define GL_EXPECT(lhs, rhs, ...)
#define GL_GET_EXPECT(pname, rhs, ...)
#define GL_VERIFY_NOT_REACHED(...)
#endif

#ifdef VALIDATE_MATH
#define MATH_VERIFY(cond, ...) { if(!(cond)) log("MATH_VERIFY failed %s:%d: %s(): " #cond "; " __VA_ARGS__, __FILE__, __LINE__, __func__); }
#define MATH_EXPECT(lhs, rhs, ...) { float l = (lhs); float r = (rhs); if(fabs(l - r) > VALIDATE_MATH_MARGIN) log("MATH_EXPECT failed: {}:{}: {}(): " #lhs " {{{}}} == " #rhs " {{{}}}; " __VA_ARGS__, __FILE__, __LINE__, __func__, l, r); }
#define MATH_EXPECT_NORMALIZED(v, ...) { float len = v.length(); if(fabs(len - 1.0f) > VALIDATE_MATH_MARGIN) log("MATH_EXPECT_NORMALIZED failed: {}:{}: {}(): " #v " {{{}}}; " __VA_ARGS__, __FILE__, __LINE__, __func__, len); }
#define MATH_VARIFY_NOT_REACHED(...) MATH_VERIFY(false, __VA_ARGS__)
#else
#define MATH_VARIFY(cond, ...)
#define MATH_VARIFY_NOT_REACHED(...)
#define MATH_EXPECT(lhs, rhs, ...)
#define MATH_EXPECT_NORMALIZED(v, ...)
#endif
