#include <windows.h>
#include "TestRenderer.hpp"
#include "engine.hpp"
#include "math/Vec3.hpp"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow){
	TestRenderer renderer;
	return engine_main(&renderer, hInstance, hPrevInstance, lpCmdLine, nCmdShow);
}