#include "Window.hpp"
#include "opengl/core.hpp"
#include "platform/windows/WindowImpl.hpp"
#include "util/log.hpp"
#include "util/target.hpp"


MAKE_WEAK_SYMBOL(onNoInitialWindow)
extern "C" void WEAK_SYMBOL_IMPL(onNoInitialWindow)() {
    onInitialWindowConstructed(plt::Window::construct());
}

struct MyCallback : plt::WindowStateCallback {
    void onInitialize(plt::Window*) override {
    }
};

namespace ui {
void test();
}

MAKE_WEAK_SYMBOL(onInitialWindowConstructed)
extern "C" void WEAK_SYMBOL_IMPL(onInitialWindowConstructed)(std::unique_ptr<plt::Window> window) {
    MyCallback callback;
    window->setStateCallback(&callback);
    // window->setDimensions(200, 200);
    // window->setTitle("Hello World!");
    ui::test();

    /*gl::initialize();
    gl::attachWindow(window.get());
    window->initialize();
    while (!window->destoryed()) {
        plt::processEvents();
        glViewport(0, 0, window->width(), window->height());
        glClearColor(1, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        SwapBuffers(static_cast<plt::windows::WindowImpl*>(window.get())->deviceContext());
    }*/
}