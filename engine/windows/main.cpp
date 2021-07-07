#include <windows.h>
#include "windows/engine.hpp"
#include "main.hpp"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    struct CreateFunctions functions;
    #define __FOREACH_G_API(api) functions.createRenderer_##api = createRenderer<api>;
    ENUMERATE_G_APIS(__FOREACH_G_API)
    #undef __FOREACH_G_API
    return engine_main(functions, hInstance, hPrevInstance, lpCmdLine, nCmdShow);
}
