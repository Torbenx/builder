#include "windows/engine.hpp"
#include "GL/Shader.hpp"
#include "GL/glwrapper.hpp"
#include "Renderer.hpp"
#include "input/keycodes.hpp"
#include "util/log.hpp"
#include "windows/fullscreen.hpp"
#include "windows/redirect.hpp"
#include <Windows.h>
#include <Windowsx.h>
#include <ctime>
#include <list>
#include <map>
#include <stdio.h>

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.hpp>

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
std::map<unsigned int, unsigned int> counter;

void init_text();
void init_time();
void init_textureFormats();

void gl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
    if (severity != GL_DEBUG_SEVERITY_NOTIFICATION)
        LOGD("glDebug: [%d]: %s", severity, message);
}

void initializeGL(const HDC& hdc) {
    HGLRC hglrc_initial = wglCreateContext(hdc);
    wglMakeCurrent(hdc, hglrc_initial);
    glwrapper_init();

    /*if(wglSwapIntervalEXT(0)){
        LOGD("disabled vsync");
    }*/

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
}

void initializeVK() {
    auto appInfo = vk::ApplicationInfo()
                       .setPApplicationName("Engine")
                       .setApplicationVersion(0)
                       .setPEngineName("Engine")
                       .setEngineVersion(0)
                       .setApiVersion(VK_API_VERSION_1_0);

    const char* layers[] = { "VK_LAYER_KHRONOS_validation" };
    const char* extensions[] = { VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME };
    auto instanceInfo = vk::InstanceCreateInfo()
                            .setPApplicationInfo(&appInfo)
                            .setEnabledLayerCount(1)
                            .setPpEnabledLayerNames(layers)
                            .setEnabledExtensionCount(2)
                            .setPpEnabledExtensionNames(extensions);
    vk::Instance inst;
    auto result = vk::createInstance(&instanceInfo, nullptr, &inst);

    uint32_t count = 0;
    result = inst.enumeratePhysicalDevices(&count, nullptr);
    if (count != 1) {
        return;
    }
    vk::PhysicalDevice device;
    result = inst.enumeratePhysicalDevices(&count, &device);
    auto properties = device.getProperties();
    LOGD("device: %s, type: %s", properties.deviceName.data(), to_string(properties.deviceType).c_str());

    auto surfaceInfo = vk::Win32SurfaceCreateInfoKHR()
                           .setHinstance(engine_hinstance)
                           .setHwnd(engine_hwnd);
    vk::SurfaceKHR surface;
    result = inst.createWin32SurfaceKHR(&surfaceInfo, nullptr, &surface);

    uint32_t familyCount = 0;
    device.getQueueFamilyProperties(&familyCount, nullptr);
    vk::QueueFamilyProperties familyProperties[familyCount];
    device.getQueueFamilyProperties(&familyCount, familyProperties);

    for (int_t i = 0; i < familyCount; i++) {
        vk::Bool32 supportsPresent = VK_FALSE;
        result = device.getSurfaceSupportKHR(i, surface, &supportsPresent);
        LOGD("i=%lld, supportsPresent=%d, graphics=%d", i, supportsPresent, (bool)(familyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics));
    }

    LOGD("%s", result == vk::Result::eSuccess ? "success" : "fail");

    while (true) { }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    /*counter[msg]++;
    if(counter[msg] - counter[msg] / 100 * 100 == 0){
        LOGD("0x%x: %u", msg, counter[msg]);
    }*/
    switch (msg) {
    case WM_CREATE: {
        LOGD("WM_CREATE");

        HDC hdc = GetDC(hwnd);

        PIXELFORMATDESCRIPTOR pfd = {
            sizeof(PIXELFORMATDESCRIPTOR),
            1,
            PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER, //Flags
            PFD_TYPE_RGBA, //The kind of framebuffer. RGBA or palette.
            32, //Colordepth of the framebuffer.
            0, 0, 0, 0, 0, 0,
            0,
            0,
            0,
            0, 0, 0, 0,
            16, //Number of bits for the depthbuffer
            0, //Number of bits for the stencilbuffer
            0, //Number of Aux buffers in the framebuffer.
            PFD_MAIN_PLANE,
            0,
            0, 0, 0
        };

        SetPixelFormat(hdc, ChoosePixelFormat(hdc, &pfd), &pfd);

        //initializeGL(hdc);
        initializeVK();

        ID3D11DeviceContext* dxContext;
        D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, D3D11_CREATE_DEVICE_BGRA_SUPPORT, NULL, 0, D3D11_SDK_VERSION, &dxDevice, NULL, &dxContext);
        hDxDevice = wglDXOpenDeviceNV(dxDevice);
        init_text();

        renderer = createFunctions.createRenderer_OpenGL();
        return 0;
    }
    case WM_SIZE: {
        LOGD("WM_SIZE");
        RECT rect;
        if (GetClientRect(hwnd, &rect)) {
            if (rect.right != 0 && rect.bottom != 0) {
                renderer->change(rect.right, rect.bottom);
            }
        } else {
            LOGW("GetClientRect failed");
        }
        return 0;
    }
    case WM_ACTIVATE: {
        LOGD("WM_ACTIVATE: %llx", wParam);
        bool activate = LOWORD(wParam);
        bool minimized = HIWORD(wParam);
        if (activate && !minimized) {
            if (rendererState == RENDERER_STATE_INVISIBLE) {
                rendererState = RENDERER_STATE_BACKGROUND;
                renderer->visible();
            }
            rendererState = RENDERER_STATE_FOREGROUND;
            renderer->foreground();
            //ShowCursor(false);
            engine_goForeground();
        } else {
            if (rendererState == RENDERER_STATE_FOREGROUND) {
                rendererState = RENDERER_STATE_BACKGROUND;
                renderer->background();
            }
            //ShowCursor(true);
            bool visible = engine_goBackground();
            if (((!visible) || minimized) && rendererState == RENDERER_STATE_BACKGROUND) {
                rendererState = RENDERER_STATE_INVISIBLE;
                renderer->invisible();
            }
        }
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    case WM_PAINT:
        //LOGD("WM_PAINT");
        if (rendererState != RENDERER_STATE_INVISIBLE) {
            renderer->drawFrame();
            SwapBuffers(GetDC(hwnd));
            return 0;
        } else {
            return DefWindowProc(hwnd, msg, wParam, lParam);
        }
    case WM_INPUT:
        //LOGD("WM_INPUT");
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
                        LOGW("switched from relative to absolute input");
                        dx = raw->data.mouse.lLastX - lastMouseX;
                        dy = raw->data.mouse.lLastY - lastMouseY;
                    } else {
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
                //LOGD("%d, %d -> %d, %d", raw->data.mouse.lLastX, raw->data.mouse.lLastY, dx, dy);
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
                    LOGD("scroll: %d", ((signed short)raw->data.mouse.usButtonData) / 120);
                }
                break;
            }
            case RIM_TYPEKEYBOARD: {
                //LOGD("key: 0x%x, flags: %d%d%d", raw->data.keyboard.VKey, raw->data.keyboard.Flags & 1, (raw->data.keyboard.Flags & 2) >> 1, (raw->data.keyboard.Flags & 4) >> 2);
                renderer->rawKey(raw->data.keyboard.VKey, !(raw->data.keyboard.Flags & RI_KEY_BREAK));
                break;
            }
            }
        }
        return DefWindowProc(hwnd, msg, wParam, lParam);
        ;
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
        LOGD("WM_CLOSE");
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        LOGD("WM_DESTROY");
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

int_t RendererState::getState() {
    return rendererState;
}

void RendererState::exit() {
    DestroyWindow(engine_hwnd);
}

int engine_main(CreateFunctions functions, HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    createFunctions = functions;
    engine_hinstance = hInstance;
    redirectToConsole();
    init_time();
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
        "", //window title
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
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        DispatchMessage(&msg);
    }

    return msg.wParam;
}

/*#include <bit>
uintptr_t allocPtr = 0;

void* operator new(std::size_t count, std::align_val_t al) {
    std::size_t alignment = (std::size_t)al;
    if (!allocPtr) {
        int_t size =  1024 * 1024 * 1024;
        allocPtr = (uintptr_t)VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        std::memset((void*)allocPtr, 0, size);
        LOGD("allocPtr=%p", allocPtr);
    }
    alignment = std::bit_ceil(alignment);
    allocPtr = (allocPtr & ~(alignment - 1)) + alignment;
    uintptr_t ret = allocPtr;
    allocPtr += count;
    //LOGD("alloc %p: size=%llu, alignment=%llu", (void*)ret, count, alignment);
    return (void*)ret;
}

void operator delete(void* ptr, std::align_val_t al) noexcept {
    std::size_t alignment = (std::size_t)al;
    //LOGD("free %p: alignment=%llu", ptr, alignment);
}

void* operator new(std::size_t count) {
    return operator new(count, (std::align_val_t)__STDCPP_DEFAULT_NEW_ALIGNMENT__);
}

void operator delete(void* ptr) noexcept {
    operator delete(ptr, (std::align_val_t)__STDCPP_DEFAULT_NEW_ALIGNMENT__);
}*/
