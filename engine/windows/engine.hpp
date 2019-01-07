#ifndef HEADER_ENGINE_H
#define HEADER_ENGINE_H

#include "../Renderer.hpp"
#include <Windows.h>
#include <D3D11.h>

extern HWND engine_hwnd;
extern ID3D11Device *dxDevice;
extern HANDLE hDxDevice;

int engine_main(Renderer *r, HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);

#endif