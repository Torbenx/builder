#pragma once

#include "Renderer.hpp"
#include <D3D11.h>
#include <Windows.h>

namespace Tag {
struct Windows {
    static constexpr char tag[] = "windows";
    static constexpr bool verifyEnabled = true;
};
}

extern HWND engine_hwnd;
extern ID3D11Device* dxDevice;
extern HANDLE hDxDevice;

int engine_main(CreateFunctions functions, HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);
