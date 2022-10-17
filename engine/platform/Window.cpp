#include "Window.hpp"
#include "containers/AtomicArrayBuilder.hpp"
#include "opengl/core.hpp"
#include "platform/windows/WindowImpl.hpp"
#include "util/log.hpp"
#include "util/target.hpp"
#include <thread>

namespace plt {

dep::Source& processedEvents() {
    static dep::Source source = {};
    return source;
}

void continueStartup(Window initialWindow) {
    int_t extraThreadCount = 10;
    std::vector<std::thread> threads;
    threads.reserve(extraThreadCount);
    for (int_t i = 0; i < extraThreadCount; i++)
        threads.emplace_back(dep::enterScheduler);

    dep::startScheduler(
        {
            &initialWindow,
            [](Window* window, dep::Scheduler scheduler) {
                if (*window)
                    onInitialWindowConstructed(*window, scheduler);
                else
                    onNoInitialWindow(scheduler);

                VERIFY(Platform, !tickComplete().unlockForResult(dep::tick()));
            },
        },
        0);

    for (std::thread& thread : threads)
        thread.join();
}

namespace {
    struct CallbackItem {
        evt::Source<evt::CallbackEvent>* postTo = nullptr;
        evt::CallbackEvent event;
    };
    using CallbackQueue = AtomicArrayBuilder<CallbackItem>;
    AtomicArrayBuilder<CallbackQueue::TakeReturnTypeView> callbacksForNextTick;
    dep::TickRingBuffer<CallbackQueue> queuedCallbacks;
}
void postCallback(evt::Source<evt::CallbackEvent>& postTo, stx::BasicCallback<> callback) {
    queuedCallbacks[dep::tick()].pushBack({ &postTo, { callback } });
}

dep::Target& tickComplete() {
    static dep::Target target { [](dep::Scheduler scheduler) {
        callbacksForNextTick.pushBack(queuedCallbacks[dep::tick()].take().take());
        scheduler.tickComplete();
    } };
    return target;
}

void processEvents(dep::Scheduler scheduler, stx::BasicCallback<> callback) {
    processEventsImpl(callback);
    evt::setCurrentExecutor(coreExe());
    auto callbacks = callbacksForNextTick.take();
    for (auto& list : callbacks) {
        for (CallbackItem c : list)
            c.postTo->sendEvent(c.event);
        list.deallocate();
    }
    coreExe()->execute();
    processedEvents().signalTargets(scheduler);
}

}

MAKE_WEAK_SYMBOL(onNoInitialWindow)
extern "C" void WEAK_SYMBOL_IMPL(onNoInitialWindow)(dep::Scheduler scheduler) {
    onInitialWindowConstructed(plt::Window::construct(), scheduler);
}

namespace ui {
void test(dep::Scheduler);
}

MAKE_WEAK_SYMBOL(onInitialWindowConstructed)
extern "C" void WEAK_SYMBOL_IMPL(onInitialWindowConstructed)(plt::Window, dep::Scheduler scheduler) {
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