#ifndef HEADER_UTIL_TARGET_H
#define HEADER_UTIL_TARGET_H

#ifdef PLATFORM_WINDOWS
#define IS_WINDOWS true
#else
#define IS_WINDOWS false
#endif

#ifdef PLATFORM_ANDROID
#define IS_ANDROID true
#else
#define IS_ANDROID false
#endif

bool isWindows();
bool isAndroid();
bool rawInput();

#endif