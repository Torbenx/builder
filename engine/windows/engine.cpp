#include "windows/engine.hpp"
#include "GL/Shader.hpp"
#include "GL/glwrapper.hpp"
#include "Renderer.hpp"
#include "input/keycodes.hpp"
#include "util/LogTimer.hpp"
#include "util/log.hpp"
#include "windows/fullscreen.hpp"
#include <Windowsx.h>
#include <ctime>
#include <iostream>
#include <list>
#include <map>
#include <stdio.h>

#define VK_USE_PLATFORM_WIN32_KHR
#include "VK/VKmain.hpp"

const char g_szClassName[] = "myWindowClass";

Renderer<OpenGL>* renderer;
CreateFunctions createFunctions;
HWND engine_hwnd;
HINSTANCE engine_hinstance;
ID3D11Device* dxDevice;
HANDLE hDxDevice;
int_t rendererState = RENDERER_STATE_INVISIBLE;
int_t lastMouseX = std::numeric_limits<int_t>::max();
int_t lastMouseY = std::numeric_limits<int_t>::max();

void init_text();
void init_time();
void init_platform();
void init_effect();
void init_textureFormats();

void gl_debug_callback(
    [[maybe_unused]] GLenum source,
    [[maybe_unused]] GLenum type,
    [[maybe_unused]] GLuint id,
    [[maybe_unused]] GLenum severity,
    [[maybe_unused]] GLsizei length,
    [[maybe_unused]] const GLchar* message,
    [[maybe_unused]] const void* userParam) {

    if (severity != GL_DEBUG_SEVERITY_NOTIFICATION)
        log<Tag::GL>("[{}] glDebug: {}", severity, message);
}

void initializeGL(HWND hwnd) {
    HDC hdc = GetDC(hwnd);

    PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR),
        1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_SWAP_EXCHANGE, // Flags
        PFD_TYPE_RGBA, // The kind of framebuffer. RGBA or palette.
        32, // Colordepth of the framebuffer.
        0, 0, 0, 0, 0, 0,
        0,
        0,
        0,
        0, 0, 0, 0,
        16, // Number of bits for the depthbuffer
        0, // Number of bits for the stencilbuffer
        0, // Number of Aux buffers in the framebuffer.
        PFD_MAIN_PLANE,
        0,
        0, 0, 0
    };
    SetPixelFormat(hdc, ChoosePixelFormat(hdc, &pfd), &pfd);

    HGLRC hglrc_initial = wglCreateContext(hdc);
    wglMakeCurrent(hdc, hglrc_initial);
    glwrapper_init();
    wglDeleteContext(hglrc_initial);

    int context_attribs[] = {
        WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
        0
    };
    HGLRC hglrc = wglCreateContextAttribsARB(hdc, NULL, context_attribs);
    wglMakeCurrent(hdc, hglrc);
    glDebugMessageCallbackARB((GLDEBUGPROCARB)gl_debug_callback, nullptr);
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

    /*if (wglSwapIntervalEXT(0)) {
        log<Tag::GL>("disabled vsync");
    }*/

    ReleaseDC(hwnd, hdc);
}

void engine_draw() {
    if (rendererState != RENDERER_STATE_INVISIBLE) {

        renderer->drawFrame();

        HDC hdc = GetDC(engine_hwnd);
        SwapBuffers(hdc);
        ReleaseDC(engine_hwnd, hdc);
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        log<Tag::Windows>("WM_CREATE");

        initializeGL(hwnd);

        ID3D11DeviceContext* dxContext;
        D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, D3D11_CREATE_DEVICE_BGRA_SUPPORT, NULL, 0, D3D11_SDK_VERSION, &dxDevice, NULL, &dxContext);
        hDxDevice = wglDXOpenDeviceNV(dxDevice);
        init_text();

        /*VulkanState::initialize(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);

        auto surfaceInfo = vk::Win32SurfaceCreateInfoKHR()
                               .setHinstance(engine_hinstance)
                               .setHwnd(engine_hwnd);
        vk::SurfaceKHR surface;
        VULKAN_VERIFY(VulkanState::instance.createWin32SurfaceKHR(&surfaceInfo, nullptr, &surface));
        VulkanState::initializeSurface(surface);*/

        renderer = createFunctions.createRenderer_OpenGL();
        return 0;
    }
    case WM_SIZE: {
        log<Tag::Windows>("WM_SIZE");

        /*auto surfaceInfo = vk::Win32SurfaceCreateInfoKHR()
                               .setHinstance(engine_hinstance)
                               .setHwnd(engine_hwnd);
        vk::SurfaceKHR surface;
        VULKAN_VERIFY(VulkanState::instance.createWin32SurfaceKHR(&surfaceInfo, nullptr, &surface));
        VulkanState::updateSurface(surface);*/

        RECT rect;
        if (GetClientRect(hwnd, &rect)) {
            if (rect.right != 0 && rect.bottom != 0) {
                renderer->change(rect.right, rect.bottom);
            }
        } else {
            log<Tag::Windows>("GetClientRect failed");
        }
        return 0;
    }
    case WM_ACTIVATE: {
        log<Tag::Windows>("WM_ACTIVATE: {:#b}", wParam);
        bool activate = LOWORD(wParam);
        bool minimized = HIWORD(wParam);
        VERIFY(Windows, !(activate && minimized));
        if (activate) {
            if (rendererState == RENDERER_STATE_INVISIBLE) {
                rendererState = RENDERER_STATE_BACKGROUND;
                renderer->visible();
            }
            rendererState = RENDERER_STATE_FOREGROUND;
            renderer->foreground();
            engine_goForeground();
        } else {
            if (rendererState == RENDERER_STATE_FOREGROUND) {
                rendererState = RENDERER_STATE_BACKGROUND;
                renderer->background();
            }
            engine_goBackground();
            if (minimized && rendererState == RENDERER_STATE_BACKGROUND) {
                rendererState = RENDERER_STATE_INVISIBLE;
                renderer->invisible();
            }
        }
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    case WM_INPUT:
        if (rendererState == RENDERER_STATE_FOREGROUND) {
            UINT size;
            GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &size, sizeof(RAWINPUTHEADER));
            std::vector<BYTE> data(size);
            GetRawInputData((HRAWINPUT)lParam, RID_INPUT, data.data(), &size, sizeof(RAWINPUTHEADER));
            RAWINPUT* raw = (RAWINPUT*)data.data();
            switch (raw->header.dwType) {
            case RIM_TYPEMOUSE: {
                int_t dx, dy;
                if (raw->data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE) {
                    if (lastMouseX != INT_MAX) {
                        dx = raw->data.mouse.lLastX - lastMouseX;
                        dy = raw->data.mouse.lLastY - lastMouseY;
                    } else {
                        log<Tag::Windows>("switched from relative to absolute input");
                        dx = 0;
                        dy = 0;
                    }
                    lastMouseX = raw->data.mouse.lLastX;
                    lastMouseY = raw->data.mouse.lLastY;
                } else {
                    dx = raw->data.mouse.lLastX;
                    dy = raw->data.mouse.lLastY;
                    lastMouseX = INT_MAX;
                    lastMouseY = INT_MAX;
                }
                renderer->rawPointer(dx, dy);
                unsigned short buttons = raw->data.mouse.usButtonFlags;
                if (buttons & RI_MOUSE_RIGHT_BUTTON_DOWN)
                    renderer->rawMouseKey(RIGHT_MOUSE_BUTTON, true);
                if (buttons & RI_MOUSE_RIGHT_BUTTON_UP)
                    renderer->rawMouseKey(RIGHT_MOUSE_BUTTON, false);
                if (buttons & RI_MOUSE_LEFT_BUTTON_DOWN)
                    renderer->rawMouseKey(LEFT_MOUSE_BUTTON, true);
                if (buttons & RI_MOUSE_LEFT_BUTTON_UP)
                    renderer->rawMouseKey(LEFT_MOUSE_BUTTON, false);
                if (buttons & RI_MOUSE_MIDDLE_BUTTON_DOWN)
                    renderer->rawMouseKey(MIDDLE_MOUSE_BUTTON, true);
                if (buttons & RI_MOUSE_MIDDLE_BUTTON_UP)
                    renderer->rawMouseKey(MIDDLE_MOUSE_BUTTON, false);
                if (buttons & RI_MOUSE_BUTTON_4_DOWN)
                    renderer->rawMouseKey(EXTRA_MOUSE_BUTTON(1), true);
                if (buttons & RI_MOUSE_BUTTON_4_UP)
                    renderer->rawMouseKey(EXTRA_MOUSE_BUTTON(1), false);
                if (buttons & RI_MOUSE_BUTTON_5_DOWN)
                    renderer->rawMouseKey(EXTRA_MOUSE_BUTTON(2), true);
                if (buttons & RI_MOUSE_BUTTON_5_UP)
                    renderer->rawMouseKey(EXTRA_MOUSE_BUTTON(2), false);
                if (buttons & RI_MOUSE_WHEEL) {
                    log("scroll: {}", ((signed short)raw->data.mouse.usButtonData) / 120);
                }
                break;
            }
            case RIM_TYPEKEYBOARD: {
                // log("key: {:#x}, flags: {}{}{}, raw->data.keyboard.VKey, raw->data.keyboard.Flags & 1, (raw->data.keyboard.Flags & 2) >> 1, (raw->data.keyboard.Flags & 4) >> 2);
                renderer->rawKey(raw->data.keyboard.VKey, !(raw->data.keyboard.Flags & RI_KEY_BREAK));
                break;
            }
            }
        }
        return DefWindowProc(hwnd, msg, wParam, lParam);
    case WM_MOUSEMOVE: {
        PointerEvent event;
        event.pointerId = POINTER_ID_CURSOR;
        event.event = POINTER_EVENT_MOVE;
        event.x = GET_X_LPARAM(lParam);
        event.y = GET_Y_LPARAM(lParam);
        renderer->pointer(event);
        return 0;
    }
    case WM_LBUTTONDOWN: {
        PointerEvent event;
        event.pointerId = POINTER_ID_CURSOR;
        event.event = POINTER_EVENT_DOWN;
        event.x = GET_X_LPARAM(lParam);
        event.y = GET_Y_LPARAM(lParam);
        renderer->pointer(event);
        return 0;
    }
    case WM_LBUTTONUP: {
        PointerEvent event;
        event.pointerId = POINTER_ID_CURSOR;
        event.event = POINTER_EVENT_UP;
        event.x = GET_X_LPARAM(lParam);
        event.y = GET_Y_LPARAM(lParam);
        renderer->pointer(event);
        return 0;
    }
    case WM_CLOSE:
        log<Tag::Windows>("WM_CLOSE");
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        log<Tag::Windows>("WM_DESTROY");
        if (rendererState == RENDERER_STATE_FOREGROUND) {
            rendererState = RENDERER_STATE_BACKGROUND;
            renderer->background();
        }
        if (rendererState == RENDERER_STATE_BACKGROUND) {
            rendererState = RENDERER_STATE_INVISIBLE;
            renderer->invisible();
        }
        delete renderer;
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

int engine_main(CreateFunctions functions, HINSTANCE hInstance, [[maybe_unused]] HINSTANCE hPrevInstance, [[maybe_unused]] LPSTR lpCmdLine, [[maybe_unused]] int nCmdShow) {
    createFunctions = functions;
    engine_hinstance = hInstance;
    init_time();
    init_platform();
    init_effect();
    init_textureFormats();

    const int_t rawInputDeviceCount = 2;
    RAWINPUTDEVICE rid[rawInputDeviceCount];

    rid[0].usUsagePage = 0x01;
    rid[0].usUsage = 0x02;
    rid[0].dwFlags = 0;
    rid[0].hwndTarget = 0;

    rid[1].usUsagePage = 0x01;
    rid[1].usUsage = 0x06;
    rid[1].dwFlags = 0;
    rid[1].hwndTarget = 0;
    RegisterRawInputDevices(rid, rawInputDeviceCount, sizeof(RAWINPUTDEVICE));

    WNDCLASSEX wc;
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = g_szClassName;
    wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassEx(&wc)) {
        MessageBox(NULL, "Window Registration Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    int_t width = 1024;
    int_t height = 512;
    engine_hwnd = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        g_szClassName,
        "", // window title
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, width, height,
        NULL, NULL, hInstance, NULL);

    if (engine_hwnd == NULL) {
        MessageBox(NULL, "Window Creation Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    ShowWindow(engine_hwnd, nCmdShow);
    UpdateWindow(engine_hwnd);

    MSG msg;
    do {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            DispatchMessage(&msg);
        else
            engine_draw();
    } while (msg.message != WM_QUIT);

    return msg.wParam;
}
