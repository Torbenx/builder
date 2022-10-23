#include "Window.hpp"
#include "containers/AtomicArrayBuilder.hpp"
#include "opengl/core.hpp"
#include "platform/windows/WindowImpl.hpp"
#include "util/log.hpp"
#include "util/target.hpp"
#include <thread>

MAKE_WEAK_SYMBOL(onNoInitialWindow)
extern "C" void WEAK_SYMBOL_IMPL(onNoInitialWindow)() {
    onInitialWindowConstructed(plt::Window::construct());
}

namespace ui {
void test();
}

MAKE_WEAK_SYMBOL(onInitialWindowConstructed)
extern "C" void WEAK_SYMBOL_IMPL(onInitialWindowConstructed)(plt::Window) {
    // window->setDimensions(200, 200);
    // window->setTitle("Hello World!");
    ui::test();

    /*gl::initialize();
    gl::attachWindow(window.get());
    window->initialize();
    while (!window->destroyed()) {
        plt::processEvents();
        glViewport(0, 0, window->width(), window->height());
        glClearColor(1, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        SwapBuffers(static_cast<plt::windows::WindowImpl*>(window.get())->deviceContext());
    }*/
}