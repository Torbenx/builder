#pragma once

#include "event/conditions.hpp"
#include "handles.hpp"

namespace ui {

struct ResizeEvent {
    IntVec newDimensions;
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

struct WindowRelayScript  {
    using PromiseBase = evt::EventRelayScript<WindowStateUpdate, ResizeEvent>::Promise;
    struct Promise : PromiseBase {
        WindowState currentState = WindowState::Uninitialized;
        IntVec currentDimensions;

        evt::BinaryState initialized { false };
        evt::BinaryState visible { false };
        evt::BinaryState foreground { false };
        evt::BinaryState destroyed { false };

        using PromiseBase::PromiseBase;
        WindowRelayScript get_return_object() { return { this }; }
    };
    using promise_type = Promise;
    Promise* promise = nullptr;
};

struct Window {
    RectVarHandle m_rect;

    const WindowRelayScript::Promise* relayPromise = nullptr;

    Window() = default;
    Window(const Window&) = delete;

    const evt::EventSource<WindowStateUpdate>& stateUpdate() const {
        return relayPromise->relay<WindowStateUpdate>().source;
    }
    const evt::EventSource<ResizeEvent>& resizeEvent() const {
        return relayPromise->relay<ResizeEvent>().source;
    }
    const evt::BinaryState& initialized() const { return relayPromise->initialized; }
    const evt::BinaryState& visible() const { return relayPromise->visible; }
    const evt::BinaryState& foreground() const { return relayPromise->foreground; }
    const evt::BinaryState& destroyed() const { return relayPromise->destroyed; }
    WindowState state() const { return relayPromise->currentState; }
    IntVec dimensions() const { return relayPromise->currentDimensions; }
    RectVarHandle rect() const { return m_rect; }
};

const Window& window();

}