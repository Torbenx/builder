#pragma once

#include "Renderer.hpp"
#include "GL/glwrapper.hpp"

namespace Tag {
struct Windows {
    static constexpr char tag[] = "windows";
    static constexpr bool verifyEnabled = true;
};
}

extern HWND engine_hwnd;
extern ID3D11Device* dxDevice;
extern HANDLE hDxDevice;
extern int_t rendererState;

int engine_main(CreateFunctions functions, HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);
