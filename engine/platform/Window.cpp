#include "Window.hpp"
#include "util/log.hpp"
#include "util/target.hpp"
#include "opengl/core.hpp"


MAKE_WEAK_SYMBOL(onNoInitialWindow)
void WEAK_SYMBOL_IMPL(onNoInitialWindow)() {
    onInitialWindowConstructed(plt::Window::construct());
}

struct MyCallback : plt::WindowStateCallback {
    void onInitialize(plt::Window*) override {
        
    }
};

MAKE_WEAK_SYMBOL(onInitialWindowConstructed)
void WEAK_SYMBOL_IMPL(onInitialWindowConstructed)(std::unique_ptr<plt::Window> window) {
    MyCallback callback;
    window->setStateCallback(&callback);
    //window->setDimensions(200, 200);
    //window->setTitle("Hello World!");

    gl::initialize();
    gl::attachWindow(window.get());
    window->initialize();
    while (window->initialized())
        plt::processEvents();
}