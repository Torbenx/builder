#include <Windows.h>
#include <stdio.h>
#include <ctime>
#include "../GL/glwrapper.hpp"
#include "../Renderer.hpp"
#include "../graphic/Shader.hpp"
#include "../util/log.hpp"
#include "engine.hpp"
#include <list>
#include "../util/time.hpp"
#include <map>
#include "fullscreen.hpp"
#include <Windowsx.h>

const char g_szClassName[] = "myWindowClass";

Renderer *renderer;
HWND engine_hwnd;
int rendererState = RENDERER_STATE_INVISIBLE;
int lastMouseX = INT_MAX;
int lastMouseY = INT_MAX;
std::map<unsigned int,unsigned int> counter;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam){
    /*counter[msg]++;
    if(counter[msg] - counter[msg] / 100 * 100 == 0){
        LOGD("0x%x: %u", msg, counter[msg]);
    }*/
    switch(msg) {
        case WM_CREATE:{
            LOGD("WM_CREATE");
            
            HDC hdc = GetDC(hwnd);
    
            PIXELFORMATDESCRIPTOR pfd = {
                sizeof(PIXELFORMATDESCRIPTOR),
                1,
                PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,    //Flags
                PFD_TYPE_RGBA,            //The kind of framebuffer. RGBA or palette.
                32,                        //Colordepth of the framebuffer.
                0, 0, 0, 0, 0, 0,
                0,
                0,
                0,
                0, 0, 0, 0,
                24,                        //Number of bits for the depthbuffer
                0,                        //Number of bits for the stencilbuffer
                0,                        //Number of Aux buffers in the framebuffer.
                PFD_MAIN_PLANE,
                0,
                0, 0, 0
            };

            SetPixelFormat(hdc,ChoosePixelFormat(hdc,&pfd),&pfd);
            
            HGLRC hglrc = wglCreateContext(hdc);
            wglMakeCurrent(hdc,hglrc);
            bool (*wglSwapIntervalEXT)(int) = (bool (*)(int)) wglGetProcAddress("wglSwapIntervalEXT");
            if(wglSwapIntervalEXT(0)){
                LOGD("disabled vsync");
            }
            glwrapper_init();
            
            renderer->create();
            return 0;
        }
        case WM_SIZE:{
            LOGD("WM_SIZE");
            RECT rect;
            if(GetClientRect(hwnd, &rect)){
                renderer->change(rect.right, rect.bottom);
            }else{
                LOGW("GetClientRect failed");
            }
            return 0;
        }
        case WM_ACTIVATE:{
            LOGD("WM_ACTIVATE: %llx",wParam);
            bool activate = LOWORD(wParam);
            bool minimized = HIWORD(wParam);
            if(activate && !minimized){
                if(rendererState == RENDERER_STATE_INVISIBLE){
                    rendererState = RENDERER_STATE_BACKGROUND;
                    renderer->visible();
                }
                rendererState = RENDERER_STATE_FOREGROUND;
                renderer->foreground();
                //ShowCursor(false);
                engine_goForeground();
            }else{
                if(rendererState == RENDERER_STATE_FOREGROUND){
                    rendererState = RENDERER_STATE_BACKGROUND;
                    renderer->background();
                }
                //ShowCursor(true);
                if((!engine_goBackground() || minimized) && rendererState == RENDERER_STATE_BACKGROUND){
                    rendererState = RENDERER_STATE_INVISIBLE;
                    renderer->invisible();
                }
            }
            return DefWindowProc(hwnd, msg, wParam, lParam);
        }
        case WM_PAINT:
            //LOGD("WM_PAINT");
            if(rendererState != RENDERER_STATE_INVISIBLE){
                renderer->drawFrame();
                SwapBuffers(GetDC(hwnd));
            }
            return 0;
        case WM_INPUT:
            //LOGD("WM_INPUT");
            if(rendererState == RENDERER_STATE_FOREGROUND){
                UINT size;
                GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &size, sizeof(RAWINPUTHEADER));
                BYTE data[size];
                GetRawInputData((HRAWINPUT)lParam, RID_INPUT, data, &size, sizeof(RAWINPUTHEADER));
                RAWINPUT *raw = (RAWINPUT*)data;
                if(raw->header.dwType == RIM_TYPEMOUSE){
                    int dx,dy;
                    if(raw->data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE){
                        if(lastMouseX != INT_MAX){
                            LOGW("switched from relative to absolute input");
                            dx = raw->data.mouse.lLastX - lastMouseX;
                            dy = raw->data.mouse.lLastY - lastMouseY;
                        }else{
                            dx = 0;
                            dy = 0;
                        }
                        lastMouseX = raw->data.mouse.lLastX;
                        lastMouseY = raw->data.mouse.lLastY;
                    }else{
                        dx = raw->data.mouse.lLastX;
                        dy = raw->data.mouse.lLastY;
                        lastMouseX = INT_MAX;
                        lastMouseY = INT_MAX;
                    }
                    //LOGD("%d, %d -> %d, %d", raw->data.mouse.lLastX, raw->data.mouse.lLastY, dx, dy);
                    renderer->rawPointer(dx, dy);
                }
            }
            return DefWindowProc(hwnd, msg, wParam, lParam);;
        case WM_MOUSEMOVE:
            PointerEvent event;
            event.pointerId = POINTER_ID_CURSOR;
            event.event = POINTER_EVENT_MOVE;
            event.x = GET_X_LPARAM(lParam);
            event.y = GET_Y_LPARAM(lParam);
            renderer->pointer(event);
            return 0;
        case WM_CLOSE:
            LOGD("WM_CLOSE");
            DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY:
            LOGD("WM_DESTROY");
            if(rendererState == RENDERER_STATE_FOREGROUND){
                rendererState = RENDERER_STATE_BACKGROUND;
                renderer->background();
            }
            if(rendererState == RENDERER_STATE_BACKGROUND){
                rendererState = RENDERER_STATE_INVISIBLE;
                renderer->invisible();
            }
            renderer->destroy();
            delete renderer;
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

int Renderer::getState(){
    return rendererState;
}

int engine_main(Renderer *r, HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow){
    renderer = r;
    time_init();
    
    WNDCLASSEX wc;
    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.style         = CS_OWNDC;
    wc.lpfnWndProc   = WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = g_szClassName;
    wc.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);

    if(!RegisterClassEx(&wc)){
        MessageBox(NULL, "Window Registration Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }
    
    int rawInputDeviceCount = 1;
    RAWINPUTDEVICE rid[rawInputDeviceCount];
    rid[0].usUsagePage = 0x01;
    rid[0].usUsage = 0x02;
    rid[0].dwFlags = 0;
    rid[0].hwndTarget = 0;
    
    RegisterRawInputDevices(rid, rawInputDeviceCount, sizeof(RAWINPUTDEVICE));

    int width = 1024;
    int height = 512;
    engine_hwnd = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        g_szClassName,
        "", //window title
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, width, height,
        NULL, NULL, hInstance, NULL
    );

    if(engine_hwnd == NULL){
        MessageBox(NULL, "Window Creation Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    ShowWindow(engine_hwnd, nCmdShow);
    UpdateWindow(engine_hwnd);
    
    MSG msg;
    while(GetMessage(&msg, NULL, 0, 0) > 0){
        DispatchMessage(&msg);
    }

    return msg.wParam;
}