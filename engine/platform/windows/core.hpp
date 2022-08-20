#pragma once

#define NTDDI_VERSION 0x0A000005
#define NOMINMAX 1
#include <Windows.h>
//must include Windows.h first
#include <D3D11.h>
#include <Dwrite.h>
#include <d2d1.h>
#include <d2d1_1.h>
#include <GL/gl.h>
#include <GL/glcorearb.h>
#include <GL/wglext.h>
#undef near
#undef far

namespace Tag {
    struct Windows {
        static constexpr char tag[] = "windows";
        static constexpr bool verifyEnabled = true;
    };
}

namespace plt::windows {
    constexpr char mainWindowClassName[] = "main_window_class";
    LRESULT CALLBACK mainWindowCallback(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    constinit inline HINSTANCE mainHInstance = nullptr;
    constexpr DWORD mainWindowExStyle = WS_EX_CLIENTEDGE;
    constexpr DWORD mainWindowStyle = WS_OVERLAPPEDWINDOW;
}