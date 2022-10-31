#include "WindowImpl.hpp"
#include "opengl/core.hpp"

namespace plt::windows {

#define GL_FUNCTION(type, name) name = (type)(void*)wglGetProcAddress(#name);
void initializeGLFunctions(HDC dc) {
    static constinit bool initialized = false;
    if (initialized)
        return;
    initialized = true;

    HGLRC initialRenderingContext = wglCreateContext(dc);
    WIN_VERIFY(initialRenderingContext);
    bool ret = wglMakeCurrent(dc, initialRenderingContext);
    WIN_VERIFY(ret);
    ENUMERATE_GL_FUNCTIONS
    ret = wglDeleteContext(initialRenderingContext);
    WIN_VERIFY(ret);
}
#undef GL_FUNCTION

void gl_debug_callback(
    [[maybe_unused]] GLenum source,
    [[maybe_unused]] GLenum type,
    [[maybe_unused]] GLuint id,
    [[maybe_unused]] GLenum severity,
    [[maybe_unused]] GLsizei length,
    [[maybe_unused]] const GLchar* message,
    [[maybe_unused]] const void* userParam) {

    if (severity != GL_DEBUG_SEVERITY_NOTIFICATION)
        log<Tag::Default>("[{}] glDebug: {}", severity, message);
}

struct GLWindowSurface : WindowSurfaceCallback {
    WindowImpl* window = nullptr;
    HGLRC renderingContext = nullptr;
    void initialize(HDC dc) {
        PIXELFORMATDESCRIPTOR pfd = {
            .nSize = sizeof(PIXELFORMATDESCRIPTOR),
            .nVersion = 1,
            .dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_SWAP_EXCHANGE,
            .iPixelType = PFD_TYPE_RGBA,
            .cColorBits = 32,
            .cRedBits = 0,
            .cRedShift = 0,
            .cGreenBits = 0,
            .cGreenShift = 0,
            .cBlueBits = 0,
            .cBlueShift = 0,
            .cAlphaBits = 0,
            .cAlphaShift = 0,
            .cAccumRedBits = 0,
            .cAccumGreenBits = 0,
            .cAccumBlueBits = 0,
            .cAccumAlphaBits = 0,
            .cDepthBits = 16,
            .cStencilBits = 0,
            .cAuxBuffers = 0,
            .iLayerType = PFD_MAIN_PLANE,
            .bReserved = 0,
            .dwLayerMask = 0,
            .dwVisibleMask = 0,
            .dwDamageMask = 0,
        };
        int format = ChoosePixelFormat(dc, &pfd);
        WIN_VERIFY(format != 0);
        bool ret = SetPixelFormat(dc, format, &pfd);
        WIN_VERIFY(ret);

        initializeGLFunctions(dc);
        int context_attribs[] = {
            WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB, 0
        };
        renderingContext = wglCreateContextAttribsARB(dc, NULL, context_attribs);
        WIN_VERIFY(renderingContext);
        wglMakeCurrent(dc, renderingContext);
        glDebugMessageCallbackARB((GLDEBUGPROCARB)gl_debug_callback, nullptr);
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    }
    void updateSize() { }
    void onAttach(WindowImpl* window) override {
        VERIFY(Windows, !this->window);
        this->window = window;
        if (!window->initialized)
            return;
        initialize(window->deviceContext());
        updateSize();
    }
    void onSurfaceCreate(WindowImpl* window) override {
        VERIFY(Windows, window == this->window);
        log("onSurfaceCreate");
        initialize(window->deviceContext());
    }
    void onSurfaceChange(WindowImpl* window) override {
        VERIFY(Windows, window == this->window);
        updateSize();
    }
    void onDetach(WindowImpl* window) override {
        VERIFY(Windows, window == this->window);
        VERIFY_NOT_REACHED(Windows);
    }
};

}

#define GL_FUNCTION(type, name) type name;
ENUMERATE_GL_FUNCTIONS
#undef GL_FUNCTION

namespace gl {

void initialize() { }

void attachWindow(plt::Window window) {
    using namespace plt::windows;
    GLWindowSurface* surface = new GLWindowSurface();
    windowImpl(window)->attachSurfaceCallback(surface);
}

}