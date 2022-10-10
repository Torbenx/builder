#include "Window.hpp"
#include "containers/AtomicArrayBuilder.hpp"
#include "opengl/core.hpp"
#include "platform/windows/WindowImpl.hpp"
#include "util/log.hpp"
#include "util/target.hpp"
#include <thread>

namespace plt {

evt::dep::Source& processedEvents() {
    static evt::dep::Source source = {};
    return source;
}

void continueStartup(Window initialWindow) {
    int_t extraThreadCount = 10;
    std::vector<std::thread> threads;
    threads.reserve(extraThreadCount);
    for (int_t i = 0; i < extraThreadCount; i++)
        threads.emplace_back(evt::enterScheduler);

    evt::startScheduler(
        {
            &initialWindow,
            [](Window* window, evt::Scheduler scheduler) {
                if (*window)
                    onInitialWindowConstructed(*window, scheduler);
                else
                    onNoInitialWindow(scheduler);

                VERIFY(Platform, !tickComplete().unlockForResult(evt::tick()));
            },
        },
        0);

    for (std::thread& thread : threads)
        thread.join();
}

namespace {
    using CallbackQueue = AtomicArrayBuilder<evt::BasicCallback<>>;
    AtomicArrayBuilder<CallbackQueue::TakeReturnTypeView> callbacksForNextTick;
    evt::TickRingBuffer<CallbackQueue> queuedCallbacks;
}
void queueCallback(evt::BasicCallback<> callback) {
    queuedCallbacks[evt::tick()].pushBack(callback);
}

evt::dep::Target& tickComplete() {
    static evt::dep::Target target { [](evt::Scheduler scheduler) {
        callbacksForNextTick.pushBack(queuedCallbacks[evt::tick()].take().take());
        scheduler.tickComplete();
    } };
    return target;
}

void processEvents(evt::Scheduler scheduler, evt::BasicCallback<> callback) {
    processEventsImpl(callback);
    auto callbacks = callbacksForNextTick.take();
    for (auto& list : *callbacks) {
        for (evt::BasicCallback<> c : list)
            c();
        list.destroy();
    }
    coreExe()->execute();
    processedEvents().signalTargets(scheduler);
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