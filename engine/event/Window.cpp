#include "Window.hpp"
#include "Awaitable.hpp"

namespace evt {

void ResizeEvent::EventSource::sendEvents(const Receiver<ResizeEvent>& events) {
    if (events.hasEvent())
        currentDimensions = events.event.value().newDimensions;
    SourceImpl<ResizeEvent>::sendEvents(events);
}

void WindowStateUpdate::EventSource::incrementalStateChange(WindowState finalState) {
    using enum WindowState;
    VERIFY(Event, currentState != Destroyed);
    auto updateState = [&](WindowState newState) {
        sendEvent({ currentState, newState });
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
void WindowStateUpdate::EventSource::updateFlags() {
    WindowStateUtil util(currentState);
    initialized.set(util.initialized());
    visible.set(util.visible());
    foreground.set(util.foreground());
    destroyed.set(util.destroyed());
}
void WindowStateUpdate::EventSource::sendEvents(const Receiver<WindowStateUpdate>& events) {
    if (events.hasEvent()) {
        Base::sendEvents(events);
        currentState = events.events.back().to;
        updateFlags();
    }
}

}