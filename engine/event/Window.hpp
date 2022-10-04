#pragma once

#include "Event.hpp"
#include "conditions.hpp"
#include "ui2/IntTypes.hpp"

namespace evt {

struct ResizeEvent {
    ui::IntVec newDimensions;
};

enum class WindowState {
    Destroyed = 0,
    Uninitialized = 1,
    Invisible = 2,
    Background = 3,
    Foreground = 4,
};
struct WindowStateUtil {
    WindowState state = WindowState::Uninitialized;
    constexpr WindowStateUtil() = default;
    constexpr WindowStateUtil(WindowState state)
        : state(state) { }
    constexpr bool initialized() const { return state >= WindowState::Invisible; }
    constexpr bool visible() const { return state > WindowState::Invisible; }
    constexpr bool foreground() const { return state >= WindowState::Foreground; }
    constexpr bool destroyed() const { return state == WindowState::Destroyed; }
    constexpr std::string_view toString() const {
        switch (state) {
        case WindowState::Uninitialized:
            return "Uninitialized";
        case WindowState::Destroyed:
            return "Destroyed";
        case WindowState::Invisible:
            return "Invisible";
        case WindowState::Background:
            return "Background";
        case WindowState::Foreground:
            return "Foreground";
        default:
            return "invalid-window-state";
        }
    }
};

struct WindowStateUpdate {
    WindowState from;
    WindowState to;
};

struct WindowBackend {
    WindowState currentState = WindowState::Uninitialized;
    ui::IntVec currentDimensions;

    EventSource<WindowStateUpdate> stateUpdates;
    EventSource<ResizeEvent> resizeEvents;
    BinaryState initialized { false };
    BinaryState visible { false };
    BinaryState foreground { false };
    BinaryState destroyed { false };

    void incrementalStateChange(WindowState);
    void updateFlags();
    void performResize(ui::IntVec);
};

struct RelayedWindowBackend : WindowBackend, RelayPromise {
    EventRingBuffer<WindowStateUpdate> stateUpdateBuf;
    EventRingBuffer<ResizeEvent> resizeEventBuf;

    RelayedWindowBackend(RelayedWindowBackend*& thiz) { thiz = this; }
    auto handle() { return stdx::coroutine_handle<RelayedWindowBackend>::from_promise(*this); }
    RelayedWindowBackend* get_return_object() { return this; }
    void startRelay(Executor* exe) { RelayPromise::startRelay(exe, handle()); }
    void reschedule() { RelayPromise::reschedule(handle()); }
};

class Window {
private:
    const WindowBackend* window = nullptr;

public:
    Window() = default;
    Window(const WindowBackend* window)
        : window(window) { }

    const WindowBackend* windowBackend() const { return window; }
    const evt::EventSource<WindowStateUpdate>& stateUpdate() const {
        return window->stateUpdates;
    }
    const evt::EventSource<ResizeEvent>& resizeEvent() const {
        return window->resizeEvents;
    }
    const evt::BinaryState& initialized() const { return window->initialized; }
    const evt::BinaryState& visible() const { return window->visible; }
    const evt::BinaryState& foreground() const { return window->foreground; }
    const evt::BinaryState& destroyed() const { return window->destroyed; }
    WindowState state() const { return window->currentState; }
    ui::IntVec dimensions() const { return window->currentDimensions; }
    explicit operator bool() const { return window; }
};

RelayedWindowBackend* relayWindow(Window);

}