#include <windows.h>
#include "windows/engine.hpp"
#include "main.hpp"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow){
    Renderer *renderer = createRenderer();
    return engine_main(renderer, hInstance, hPrevInstance, lpCmdLine, nCmdShow);
}