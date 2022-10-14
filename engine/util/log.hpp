//
// Created by Torben Thaysen on 03.02.2016.
//

#pragma once

#include "util/types.hpp"
#include <fmt/core.h>
#include <type_traits>

namespace Tag {
struct Default {
    static constexpr char tag[] = "default";
    static constexpr bool verifyEnabled = true;
};
}

void log_print(const std::string& tag, const std::string& msg);

template<typename T>
concept LogTag = std::is_same_v<decltype(T::tag), const char[sizeof(T::tag)]> && sizeof(T::tag) <= 10 + 1;

template<typename T>
concept VerifyTag = LogTag<T> && std::is_same_v<decltype(T::verifyEnabled), const bool>;

template<LogTag tag = Tag::Default, typename... Args>
static void log(fmt::format_string<Args...> str, Args&&... args) {
    std::string msg = fmt::format(str, std::forward<Args>(args)...);
    log_print(tag::tag, msg);
}

void verify_failed();
template<VerifyTag tag>
constexpr void verify(bool, const char*, const char*, int, const char*) requires(!tag::verifyEnabled) { }

template<VerifyTag tag, typename T>
constexpr void expect(T, T, const char*, const char*, const char*, int, const char*) requires(!tag::verifyEnabled) { }

template<VerifyTag tag>
constexpr void verify(bool cond, const char* condStr, const char* file, int line, const char* func) requires tag::verifyEnabled {
    if (!cond) {
        log<tag>("VERIFY failed {}:{}: {}(): {}", file, line, func, condStr);
        verify_failed();
    }
}

template<VerifyTag tag, typename F>
constexpr void expect(F lhs, F rhs, const char* lhsStr, const char* rhsStr, const char* file, int line, const char* func) requires std::is_floating_point_v<F> && tag::verifyEnabled {
    if (std::abs(lhs - rhs) > 0.00001f) {
        log<tag>("EXPECT failed {}:{}: {}(): {} {{{}}} == {} {{{}}}", file, line, func, lhsStr, lhs, rhsStr, rhs);
    }
}

template<VerifyTag tag, typename T>
constexpr void expect(const T& lhs, const T& rhs, const char* lhsStr, const char* rhsStr, const char* file, int line, const char* func) requires tag::verifyEnabled &&(!std::is_floating_point_v<T>) {
    if (lhs != rhs) {
        log<tag>("EXPECT failed {}:{}: {}(): {} {{{:#x}}} == {} {{{:#x}}}", file, line, func, lhsStr, lhs, rhsStr, rhs);
    }
}

#define VERIFY(tag, cond) verify<Tag::tag>(cond, #cond, __FILE__, __LINE__, __func__)
#define EXPECT(tag, lhs, rhs) expect<Tag::tag>(lhs, rhs, #lhs, #rhs, __FILE__, __LINE__, __func__)
#define VERIFY_NOT_REACHED(tag) verify<Tag::tag>(false, "should not be reached", __FILE__, __LINE__, __func__)
#define VDUMP(var, ...) log("{}():{}: " #var " = {" __VA_ARGS__ "}", __func__, __LINE__, var)
#define ADUMP(array, index, ...) log("{}(): " #array "[" #index " = {}] = {" __VA_ARGS__ "}", __func__, index, array[index])
