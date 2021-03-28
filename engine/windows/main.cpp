#include <windows.h>
#include "windows/engine.hpp"
#include "main.hpp"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow){
    return engine_main(createRenderer, hInstance, hPrevInstance, lpCmdLine, nCmdShow);
}
