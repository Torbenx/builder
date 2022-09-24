#pragma once

#define NTDDI_VERSION 0x0A000005
#define NOMINMAX 1
#include <Windows.h>
// must include Windows.h first
#include <D3D11.h>
#include <Dwrite.h>
#include <GL/gl.h>
#include <GL/glcorearb.h>
#include <GL/wglext.h>
#include <d2d1.h>
#include <d2d1_1.h>

#include "util/log.hpp"

#undef near
#undef far

namespace Tag {
struct Windows {
    static constexpr char tag[] = "windows";
    static constexpr bool verifyEnabled = true;
};
}

namespace plt::windows {

constinit inline HINSTANCE mainHInstance = nullptr;
constexpr char mainWindowClassName[] = "main_window_class";
constexpr DWORD mainWindowExStyle = WS_EX_CLIENTEDGE;
constexpr DWORD mainWindowStyle = WS_OVERLAPPEDWINDOW;
LRESULT CALLBACK mainWindowCallback(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

std::string_view errorName(DWORD);
std::string_view errorString(DWORD);
inline void win_verify(bool cond, const char* condStr, const char* file, int line, const char* func) {
    if constexpr (Tag::Windows::verifyEnabled) {
        if (cond)
            return;
        DWORD lastError = GetLastError();
        log<Tag::Windows>("VERIFY failed {}:{}: {}(): {}, {}({}) '{}'", file, line, func, condStr, errorName(lastError), lastError, errorString(lastError));
    }
}
#define WIN_VERIFY(cond) plt::windows::win_verify(cond, #cond, __FILE__, __LINE__, __func__)

constinit inline uint64_t performenceCounterFrequency = 0;

}