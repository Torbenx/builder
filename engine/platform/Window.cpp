#include "Window.hpp"
#include "util/log.hpp"
#include "util/target.hpp"


MAKE_WEAK_SYMBOL(onNoInitialWindow)
void WEAK_SYMBOL_IMPL(onNoInitialWindow)() {
    onInitialWindowConstructed(plt::Window::construct());
}

MAKE_WEAK_SYMBOL(onInitialWindowConstructed)
void WEAK_SYMBOL_IMPL(onInitialWindowConstructed)(std::unique_ptr<plt::Window> window) {
    window->setDimensions(200, 200);
    window->initialize();
    window->setDimensions(100, 100);
    window->setTitle("Hello World!");
    while (window->initialized())
        plt::pumpEvents();
}