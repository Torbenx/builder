#pragma once

#include "core.hpp"
#include "platform/Window.hpp"

namespace plt::windows {

struct WindowImpl;
struct WindowSurfaceCallback {
    virtual void onAttach(WindowImpl*) { }
    virtual void onSurfaceCreate(WindowImpl*) { }
    virtual void onSurfaceChange(WindowImpl*) { }
    virtual void onSurfaceDestroy(WindowImpl*) { }
    virtual void onDetach(WindowImpl*) { }
};

struct ScopedDeviceContext {
    HWND m_hwnd = nullptr;
    HDC m_hdc = nullptr;
    ScopedDeviceContext(HWND hwnd)
        : m_hwnd(hwnd), m_hdc(GetDC(hwnd)) { }
    ScopedDeviceContext(const ScopedDeviceContext& other) = delete;
    ScopedDeviceContext(ScopedDeviceContext&& other) = delete;
    ScopedDeviceContext& operator=(const ScopedDeviceContext&) = delete;
    ScopedDeviceContext& operator=(ScopedDeviceContext&&) = delete;
    ~ScopedDeviceContext() {
        if (hdc())
            ReleaseDC(hwnd(), hdc());
    }
    HDC hdc() const { return m_hdc; }
    HWND hwnd() const { return m_hwnd; }
    operator HDC() const { return hdc(); }
};

struct WindowImpl : Window {
    HWND m_hwnd = nullptr;
    std::string m_title;
    int_t m_width = 0, m_height = 0;
    WindowState m_state = WindowState::Uninitialized;
    WindowStateCallback* m_stateCallback = nullptr;
    WindowSurfaceCallback* m_surfaceCallback = nullptr;
    bool inSizeMove = false;

    std::pair<int_t, int_t> calculateWindowSize();
    void updateDimensions();
    void initialize();
    void destroy();
    void destroyImpl();
    void destructor();
    void toState(WindowState targetState);
    void setStateCallback(WindowStateCallback* callback);
    void attachSurfaceCallback(WindowSurfaceCallback* callback);
    void detachSurfaceCallback(WindowSurfaceCallback* callback);
    void setTitle(std::string_view title);
    void setDimensions(int_t width, int_t height);

    void enterSizeMove();
    void exitSizeMove();

    HWND hwnd() { return m_hwnd; }
    ScopedDeviceContext deviceContext() { return { hwnd() }; }
    std::string_view title() const { return m_title; }
    int_t width() const { return m_width; }
    int_t height() const { return m_height; }
    WindowState state() const { return m_state; }
    WindowStateCallback* stateCallback() const { return m_stateCallback; }
    WindowSurfaceCallback* surfaceCallback() const { return m_surfaceCallback; }
};

}