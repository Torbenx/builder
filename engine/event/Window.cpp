#include "Window.hpp"
#include "Awaitable.hpp"

namespace evt {

void WindowBackend::incrementalStateChange(WindowState finalState) {
    using enum WindowState;
    VERIFY(Event, currentState != Destroyed);
    auto updateState = [&](WindowState newState) {
        stateUpdates.sendEvent({ currentState, newState });
        currentState = newState;
    };

    WindowState targetState = finalState;
    if (finalState == Destroyed)
        targetState = WindowStateUtil(currentState).initialized() ? Invisible : Uninitialized;

    std::underlying_type_t<WindowState> direction = targetState > currentState ? 1 : -1;
    while (targetState != currentState)
        updateState(WindowState { stx::to_underlying(currentState) + direction });

    if (finalState == Destroyed)
        updateState(Destroyed);

    updateFlags();
}
void WindowBackend::updateFlags() {
    WindowStateUtil util(currentState);
    initialized.set(util.initialized());
    visible.set(util.visible());
    foreground.set(util.foreground());
    destroyed.set(util.destroyed());
}
void WindowBackend::performResize(ui::IntVec newDimensions) {
    resizeEvents.sendEvent({ newDimensions });
}

namespace {
    RelayedWindowBackend* broadcastWindowEvents(RelayedWindowBackend* relay = {}) {
        while (true) {
            std::vector<WindowStateUpdate> stateUpdates = std::move(relay->stateUpdateBuf[tick()]);
            relay->stateUpdates.sendEvents(stateUpdates);
            if (!stateUpdates.empty()) {
                WindowState finalState = stateUpdates.back().to;
                relay->currentState = finalState;
                relay->updateFlags();
            }

            std::vector<ResizeEvent> resizeEvents = std::move(relay->resizeEventBuf[tick()]);
            relay->resizeEvents.sendEvents(resizeEvents);
            if (!resizeEvents.empty())
                relay->currentDimensions = resizeEvents.back().newDimensions;

            relay->reschedule();
            co_await stdx::suspend_always();
        }
    }
    Script recordWindowEvents(RelayedWindowBackend* output, Window window) {
        while (true) {
            EventReceiver<WindowStateUpdate> stateUpdates;
            EventReceiver<ResizeEvent> resizeEvents;
            co_await (window.stateUpdate() > stateUpdates || window.resizeEvent() > resizeEvents);
            output->stateUpdateBuf[tick()] = std::move(stateUpdates.events);
            output->resizeEventBuf[tick()] = std::move(resizeEvents.events);
        }
    }
}
WindowRelay relayWindow(Window window) {
    RelayedWindowBackend* output = broadcastWindowEvents();
    Script input = recordWindowEvents(output, window);
    return { input, output };
}

}