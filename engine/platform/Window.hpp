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
    Uninitialized = 0,
    Destroyed = 1,
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

struct GraphicsCallbacks {
    virtual void onSurfaceCreated(Window*) { }
    virtual void onSurfaceChanged(Window*) { }
    virtual void onSurfaceLost(Window*) { }
};
struct StateCallbacks {
    virtual void onInitialize(Window*) { }
    virtual void onVisible(Window*) { }
    virtual void onForeground(Window*) { }
    virtual void onBackground(Window*) { }
    virtual void onInvisible(Window*) { }
    virtual void onDestroy(Window*) { }
    virtual void onStageChanged(Window* window, WindowState newState, WindowState oldState) {
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
    int_t width() const;
    int_t height() const;
    void setDimensions(int_t width, int_t height);
    WindowState state() const;
    void initialize();
    void destroy();
    bool initialized() const { return state() >= WindowState::Invisible; }
    bool visible() const { return state() > WindowState::Invisible; }
    bool foreground() const { return state() >= WindowState::Foreground; }
    Window(const Window&) = delete;
    ~Window();

protected:
    Window() { }
};

void pumpEvents();

}

void WEAK_SYMBOL_DECL(onNoInitialWindow)();
void WEAK_SYMBOL_DECL(onInitialWindowConstructed)(std::unique_ptr<plt::Window> window);