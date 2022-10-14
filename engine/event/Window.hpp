#pragma once

#include "Event.hpp"
#include "conditions.hpp"
#include "ui2/IntTypes.hpp"

namespace evt {

struct ResizeEvent {
    using EventReceiver = KeepLastReceiver<ResizeEvent>;
    struct EventSource : SourceImpl<ResizeEvent> {
        ui::IntVec currentDimensions;
        void sendEvents(const Receiver<ResizeEvent>& events);
        void performResize(ui::IntVec newDim) {
            sendEvent({ newDim });
        }

    private:
        using SourceImpl<ResizeEvent>::sendEvent;
    };
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
    struct EventSource : SourceImpl<WindowStateUpdate> {
        using Base = SourceImpl<WindowStateUpdate>;
        WindowState currentState = WindowState::Uninitialized;
        BinaryState initialized { false };
        BinaryState visible { false };
        BinaryState foreground { false };
        BinaryState destroyed { false };

        void sendEvents(const Receiver<WindowStateUpdate>&);
        void incrementalStateChange(WindowState);
        void updateFlags();

    private:
        using Base::sendEvent;
    };
    WindowState from;
    WindowState to;
};
static_assert(HasCustomSource<WindowStateUpdate>);

}