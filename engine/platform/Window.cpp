#include "Window.hpp"
#include "opengl/core.hpp"
#include "platform/windows/WindowImpl.hpp"
#include "util/log.hpp"
#include "util/target.hpp"
#include <thread>

namespace plt {

evt::ExecutorImpl* coreExe() {
    static evt::ExecutorImpl exe = {};
    return &exe;
}
evt::dep::Source& processedEvents() {
    static evt::dep::Source source = {};
    return source;
}

void continueStartup(Window initialWindow) {
    std::thread(evt::enterScheduler).detach();
    evt::startScheduler(
        {
            &initialWindow,
            [](Window* window, evt::Scheduler scheduler) {
                if (*window)
                    onInitialWindowConstructed(*window, scheduler);
                else
                    onNoInitialWindow(scheduler);
            },
        },
        0);
}

}

MAKE_WEAK_SYMBOL(onNoInitialWindow)
extern "C" void WEAK_SYMBOL_IMPL(onNoInitialWindow)(evt::Scheduler scheduler) {
    onInitialWindowConstructed(plt::Window::construct(), scheduler);
}

namespace ui {
void test(evt::Scheduler);
}

MAKE_WEAK_SYMBOL(onInitialWindowConstructed)
extern "C" void WEAK_SYMBOL_IMPL(onInitialWindowConstructed)(plt::Window, evt::Scheduler scheduler) {
    // window->setDimensions(200, 200);
    // window->setTitle("Hello World!");
    ui::test(scheduler);

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