#pragma once

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
}

namespace plt {

enum class WindowState {
    Destroyed = 0,
    Uninitialized = 1,
    Invisible = 2,
    Background = 3,
    Foreground = 4,
};
constexpr std::string_view windowStateToString(WindowState state) {
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

class Window;
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
        log("state change: {} -> {}", windowStateToString(oldState), windowStateToString(newState));
        if (newState == WindowState::Invisible && oldState == WindowState::Uninitialized)
            return onInitialize(window);
        if (newState == WindowState::Background && oldState == WindowState::Invisible)
            return onVisible(window);
        if (newState == WindowState::Foreground && oldState == WindowState::Background)
            return onForeground(window);
        if (newState == WindowState::Background && oldState == WindowState::Foreground)
            return onBackground(window);
        if (newState == WindowState::Invisible && oldState == WindowState::Background)
            return onInvisible(window);
        if (newState == WindowState::Destroyed && oldState == WindowState::Invisible)
            return onDestroy(window);
        log<Tag::Window>("bad window state change: {} -> {}", windowStateToString(oldState), windowStateToString(newState));
    }
};

class Window {
public:
    static std::unique_ptr<Window> construct();

    void setTitle(std::string_view title);
    std::string_view title() const;

    void setStateCallback(WindowStateCallback* callback);
    WindowStateCallback* stateCallback() const;

    void setDimensions(int_t width, int_t height);
    int_t width() const;
    int_t height() const;

    void initialize();
    void destroy();
    WindowState state() const;
    bool initialized() const { return state() >= WindowState::Invisible; }
    bool visible() const { return state() > WindowState::Invisible; }
    bool foreground() const { return state() >= WindowState::Foreground; }
    bool destroyed() const { return state() == WindowState::Destroyed; }

    Window(const Window&) = delete;
    ~Window();

protected:
    Window() { }
};

void processEvents(std::function<void()>);

}

extern "C" void WEAK_SYMBOL_DECL(onNoInitialWindow)();
extern "C" void WEAK_SYMBOL_DECL(onInitialWindowConstructed)(std::unique_ptr<plt::Window> window);