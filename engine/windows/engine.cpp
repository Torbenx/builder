#include <windows.h>
#include <stdio.h>
#include <ctime>
#include "../GL/glwrapper.hpp"
#include "../Renderer.hpp"
#include "../graphic/Shader.hpp"
#include "../util/log.hpp"
#include "engine.hpp"
#include <list>
#include "../util/time.hpp"

const char g_szClassName[] = "myWindowClass";

bool equalDEVMODE(DEVMODE first, DEVMODE second){
    return first.dmPelsWidth == second.dmPelsWidth && first.dmPelsHeight == second.dmPelsHeight && first.dmDisplayFrequency == second.dmDisplayFrequency;
}

bool equalDEVMODEIgnoreDisplayFrequency(DEVMODE first, DEVMODE second){
    return first.dmPelsWidth == second.dmPelsWidth && first.dmPelsHeight == second.dmPelsHeight;
}

bool equalDEVMODEIgnoreResolution(DEVMODE first, DEVMODE second){
    return first.dmDisplayFrequency == second.dmDisplayFrequency;
}

bool lessThenDEVMODEResolution(DEVMODE first, DEVMODE second){
    if(first.dmPelsWidth < second.dmPelsWidth)return true;
    if(first.dmPelsWidth > second.dmPelsWidth)return false;
    if(first.dmPelsHeight < second.dmPelsHeight)return true;
    if(first.dmPelsHeight > second.dmPelsHeight)return false;
    return false;
}

bool lessThenDEVMODEDisplayFrequency(DEVMODE first, DEVMODE second){
    if(first.dmDisplayFrequency < second.dmDisplayFrequency)return true;
    return false;
}

Renderer *renderer;
DEVMODE engine_devMode;
std::string engine_adapterName;
HWND engine_hwnd;

void engine_fullscreen(HWND hwnd){
    //return;
    SetWindowLongPtr(hwnd, GWL_EXSTYLE, WS_EX_APPWINDOW);
    SetWindowLongPtr(hwnd, GWL_STYLE, WS_POPUP);
    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, engine_devMode.dmPelsWidth, engine_devMode.dmPelsHeight, 0);
    LOGD("setting %s to: %ldx%ld@%ld",engine_adapterName.c_str(),engine_devMode.dmPelsWidth,engine_devMode.dmPelsHeight,engine_devMode.dmDisplayFrequency);
    long ret = ChangeDisplaySettingsEx(engine_adapterName.c_str(), &engine_devMode, NULL, CDS_FULLSCREEN, NULL);
    switch(ret){
        case DISP_CHANGE_SUCCESSFUL: LOGD("DISP_CHANGE_SUCCESSFUL"); break;
        case DISP_CHANGE_BADDUALVIEW: LOGW("DISP_CHANGE_BADDUALVIEW"); break;
        case DISP_CHANGE_BADFLAGS: LOGW("DISP_CHANGE_BADFLAGS"); break;
        case DISP_CHANGE_BADMODE: LOGW("DISP_CHANGE_BADMODE"); break;
        case DISP_CHANGE_BADPARAM: LOGW("DISP_CHANGE_BADPARAM"); break;
        case DISP_CHANGE_FAILED: LOGW("DISP_CHANGE_FAILED"); break;
        case DISP_CHANGE_NOTUPDATED: LOGW("DISP_CHANGE_NOTUPDATED"); break;
        case DISP_CHANGE_RESTART: LOGW("DISP_CHANGE_RESTART"); break;
        default: LOGW("unknown return value: %ld", ret);
    }
    ShowWindow(hwnd, SW_SHOW);
}

void engine_endFullscreen(HWND hwnd){
    //return;
    //SetWindowLongPtr(hwnd, GWL_EXSTYLE, 0);
    //SetWindowLongPtr(hwnd, GWL_STYLE, 0);
    //SetWindowPos(hwnd, 0, 0, 0, 0, 0, 0);
    long ret = ChangeDisplaySettingsEx(engine_adapterName.c_str(), NULL, NULL, 0, NULL);
    switch(ret){
        case DISP_CHANGE_SUCCESSFUL: LOGD("DISP_CHANGE_SUCCESSFUL"); break;
        case DISP_CHANGE_BADDUALVIEW: LOGW("DISP_CHANGE_BADDUALVIEW"); break;
        case DISP_CHANGE_BADFLAGS: LOGW("DISP_CHANGE_BADFLAGS"); break;
        case DISP_CHANGE_BADMODE: LOGW("DISP_CHANGE_BADMODE"); break;
        case DISP_CHANGE_BADPARAM: LOGW("DISP_CHANGE_BADPARAM"); break;
        case DISP_CHANGE_FAILED: LOGW("DISP_CHANGE_FAILED"); break;
        case DISP_CHANGE_NOTUPDATED: LOGW("DISP_CHANGE_NOTUPDATED"); break;
        case DISP_CHANGE_RESTART: LOGW("DISP_CHANGE_RESTART"); break;
        default: LOGW("unknown return value: %ld", ret);
    }
    ShowWindow(hwnd, SW_MINIMIZE);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam){
    switch(msg) {
        case WM_CREATE:{
            LOGD("WM_CREATE");
            int i = 0;
            DISPLAY_DEVICE adapter;
            adapter.cb = sizeof(DISPLAY_DEVICE);
            std::map<std::string,std::string> monitors;
            while(EnumDisplayDevices(nullptr, i++, &adapter, 0)){
                LOGD("name: %s, string: %s", adapter.DeviceName, adapter.DeviceString);
                int j = 0;
                DISPLAY_DEVICE monitor;
                monitor.cb = sizeof(DISPLAY_DEVICE);
                while(EnumDisplayDevices(adapter.DeviceName, j++, &monitor, 0)){
                    LOGD("    name: %s, string: %s", monitor.DeviceName, monitor.DeviceString);
                    monitors[monitor.DeviceName] = adapter.DeviceName;
                }
            }
            
            std::string monitorName = monitors.begin()->first;
            engine_adapterName = monitors[monitorName];
            std::list<DEVMODE> configs;
            int k = 0;
            DEVMODE devMode;
            devMode.dmSize = DM_SPECVERSION;
            devMode.dmSize = sizeof(DEVMODE);
            while(EnumDisplaySettings(engine_adapterName.c_str(), k++, &devMode)){
                configs.push_back(devMode);
            }
            
            std::list<DEVMODE> resolutions = configs;
            resolutions.sort(lessThenDEVMODEResolution);
            resolutions.unique(equalDEVMODEIgnoreDisplayFrequency);
            for(std::list<DEVMODE>::iterator it=resolutions.begin();it!=resolutions.end();it++){
                LOGD("%ldx%ld", it->dmPelsWidth, it->dmPelsHeight);
            }
            std::list<DEVMODE> refreshRates = configs;
            refreshRates.sort(lessThenDEVMODEDisplayFrequency);
            refreshRates.unique(equalDEVMODEIgnoreResolution);
            for(std::list<DEVMODE>::iterator it=refreshRates.begin();it!=refreshRates.end();it++){
                LOGD("%ld", it->dmDisplayFrequency);
            }
            
            DEVMODE targetConfig;
            targetConfig.dmPelsWidth = 1920;
            targetConfig.dmPelsHeight = 1080;
            targetConfig.dmDisplayFrequency = 144;
            engine_devMode.dmSize = 0;
            
            for(std::list<DEVMODE>::iterator it=configs.begin();it!=configs.end();it++){
                if(equalDEVMODE(targetConfig, *it)){
                    engine_devMode = *it;
                    break;
                }
            }
            if(engine_devMode.dmSize != 0){
                LOGD("config found for %s: %ldx%ld@%ld",engine_adapterName.c_str(),engine_devMode.dmPelsWidth,engine_devMode.dmPelsHeight,engine_devMode.dmDisplayFrequency);
            }
            
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
            
            glwrapper_init();
            
            renderer->create();
            break;
        }
        case WM_SIZE:{
            LOGD("WM_SIZE");
            RECT rect;
            if(GetClientRect(hwnd, &rect)){
                renderer->change(rect.right, rect.bottom);
            }else{
                LOGW("GetClientRect failed");
            }
            break;
        }
        case WM_ACTIVATE:
            LOGD("WM_ACTIVATE: %lld",wParam);
            if(wParam){
                engine_fullscreen(hwnd);
            }else{
                engine_endFullscreen(hwnd);
            }
            break;
        case WM_PAINT:
            //LOGD("WM_PAINT");
            renderer->drawFrame();
            SwapBuffers(GetDC(hwnd));
            break;
        case WM_SETCURSOR:
            //LOGD("WM_SETCURSOR");
            POINT point;
            GetCursorPos(&point);
            PointerEvent event;
            event.pointerId = POINTER_ID_CURSOR;
            event.event = POINTER_EVENT_MOVE;
            event.x = point.x;
            event.y = point.y;
            renderer->pointer(event);
            break;
        case WM_CLOSE:
            LOGD("WM_CLOSE");
            renderer->destroy();
            delete renderer;
            DestroyWindow(hwnd);
            break;
        case WM_DESTROY:
            LOGD("WM_DESTROY");
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
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
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return msg.wParam;
}