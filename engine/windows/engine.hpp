#ifndef HEADER_ENGINE_H
#define HEADER_ENGINE_H

#include "../Renderer.hpp"
#include <Windows.h>

extern HWND engine_hwnd;

int engine_main(Renderer *r, HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);

#endif