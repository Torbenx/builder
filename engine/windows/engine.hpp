#pragma once

#include "Renderer.hpp"
#include <D3D11.h>
#include <Windows.h>

extern HWND engine_hwnd;
extern ID3D11Device* dxDevice;
extern HANDLE hDxDevice;

#define __FOREACH_G_API(api) Renderer<api>*(*createRenderer_##api)();
struct CreateFunctions {
    ENUMERATE_G_APIS(__FOREACH_G_API)
};
#undef __FOREACH_G_API

int engine_main(CreateFunctions functions, HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);
