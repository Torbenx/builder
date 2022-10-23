#pragma once

#include "event/Window.hpp"
#include "event/Timeline.hpp"
#include "util/Callable.hpp"
#include "util/log.hpp"
#include "util/types.hpp"
#include <functional>
#include <memory>
#include <string_view>

namespace Tag {
struct Window {
    static constexpr char tag[] = "window";
    static constexpr bool verifyEnabled = true;
};
struct Platform {
    static constexpr char tag[] = "platform";
    static constexpr bool verifyEnabled = true;
};
}

namespace plt {

/*class Window;
struct WindowStateCallback {
    virtual void onAttach(Window*) { }
    virtual void onInitialize(Window*) { }
    virtual void onResize(Window*) { }
    virtual void onVisible(Window*) { }
    virtual void onForeground(Window*) { }
    virtual void onBackground(Window*) { }
    virtual void onInvisible(Window*) { }
    virtual void onDestroy(Window*) { }
    virtual void onDetach(Window*) { }
    virtual void onStageChange(Window* window, WindowState oldState, WindowState newState) {
        using enum WindowState;
        log("state change: {} -> {}", windowStateToString(oldState), windowStateToString(newState));
        if (newState == Invisible && oldState == Uninitialized)
            return onInitialize(window);
        if (newState == Background && oldState == Invisible)
            return onVisible(window);
        if (newState == Foreground && oldState == Background)
            return onForeground(window);
        if (newState == Background && oldState == Foreground)
            return onBackground(window);
        if (newState == Invisible && oldState == Background)
            return onInvisible(window);
        if (newState == Destroyed && oldState == Invisible)
            return onDestroy(window);
        log<Tag::Window>("bad window state change: {} -> {}", windowStateToString(oldState), windowStateToString(newState));
    }
};*/

struct WindowBackendBase : evt::Source<evt::WindowStateUpdate>, evt::Source<evt::ResizeEvent> { };

class Window {
public:
    static Window construct();
    Window() = default;

    void setTitle(std::string_view title);
    std::string_view title() const;

    void setDimensions(ui::IntVec dimensions);

    void initialize();
    void destroy();

    const evt::Source<evt::WindowStateUpdate>& stateUpdate() const { return *backend; }
    evt::WindowState state() const { return stateUpdate().currentState; }
    const evt::BinaryState& initialized() const { return stateUpdate().initialized; }
    const evt::BinaryState& visible() const { return stateUpdate().visible; }
    const evt::BinaryState& foreground() const { return stateUpdate().foreground; }
    const evt::BinaryState& destroyed() const { return stateUpdate().destroyed; }

    const evt::Source<evt::ResizeEvent>& resizeEvent() const { return *backend; }
    ui::IntVec dimensions() const { return resizeEvent().currentDimensions; }

    explicit operator bool() const { return (bool)backend; }

    WindowBackendBase* windowBackend() { return backend; }
    const WindowBackendBase* windowBackend() const { return backend; }

protected:
    WindowBackendBase* backend = nullptr;
    Window(WindowBackendBase* backend)
        : backend(backend) { }
};

void earlyInitialize();

}

extern "C" void WEAK_SYMBOL_DECL(onNoInitialWindow)();
extern "C" void WEAK_SYMBOL_DECL(onInitialWindowConstructed)(plt::Window window);