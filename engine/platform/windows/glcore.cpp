#include "WindowImpl.hpp"
#include "opengl/core.hpp"

namespace plt::windows {

#define GL_FUNCTION(type, name) name = (type)wglGetProcAddress(#name);
void initializeGLFunctions(HDC dc) {
    static bool initialized = false;
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
        if (!window->initialized())
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

void attachWindow(plt::Window* window) {
    using namespace plt::windows;
    GLWindowSurface* surface = new GLWindowSurface();
    static_cast<WindowImpl*>(window)->attachSurfaceCallback(surface);
}

}