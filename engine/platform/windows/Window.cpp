#include "platform/Window.hpp"
#include "core.hpp"

namespace plt::windows {
struct WindowImpl : Window {
    HWND hwnd = nullptr;
    std::string m_title;
    int_t m_width = 0, m_height = 0;
    WindowState m_state = WindowState::Uninitialized;

    std::pair<int_t, int_t> calculateWindowSize() {
        RECT rect { 0, 0, (int)width(), (int)height() };
        bool ret = AdjustWindowRectEx(&rect, mainWindowStyle, false, mainWindowExStyle);
        VERIFY(Windows, ret);
        return { rect.right - rect.left, rect.bottom - rect.top };
    }
    void updateDimensions() {
        RECT clientRect;
        GetClientRect(hwnd, &clientRect);
        m_width = clientRect.right;
        m_height = clientRect.bottom;
    }
    void initialize() {
        VERIFY(Windows, !hwnd);
        int_t targetWidth = width(), targetHeight = height();
        int windowWidth, windowHeight;
        std::tie(windowWidth, windowHeight) = calculateWindowSize();
        if (width() == 0)
            windowWidth = CW_USEDEFAULT;
        if (height() == 0)
            windowHeight = CW_USEDEFAULT;

        hwnd = CreateWindowExA(
            mainWindowExStyle, mainWindowClassName, m_title.c_str(), mainWindowStyle,
            CW_USEDEFAULT, CW_USEDEFAULT, windowWidth, windowHeight,
            NULL, NULL, mainHInstance, (void*)this);
        if (!hwnd) {
            VERIFY_NOT_REACHED(Windows);
            return;
        }

        updateDimensions();
        VERIFY(Windows, width() == targetWidth);
        VERIFY(Windows, height() == targetHeight);

        SetWindowLongPtr(hwnd, GWLP_USERDATA, (uintptr_t)this);
        ShowWindow(hwnd, SW_SHOW);
    }
    void destroy() {
        if (hwnd)
            DestroyWindow(hwnd);
        else
            destroyImpl();
    }
    void destroyImpl() {
        hwnd = nullptr;
        std::string toBeDestroyed = std::move(m_title);
        m_width = 0;
        m_height = 0;
    }
    void destructor() { destroy(); }

    void setTitle(std::string_view title) {
        m_title = title;
        if (!hwnd)
            return;
        bool ret = SetWindowText(hwnd, m_title.c_str());
        VERIFY(Windows, ret);
    }
    void setDimensions(int_t width, int_t height) {
        m_width = width;
        m_height = height;
        if (!hwnd)
            return;
        auto [windowWidth, windowHeight] = calculateWindowSize();
        bool ret = SetWindowPos(hwnd, HWND_TOPMOST, CW_USEDEFAULT, CW_USEDEFAULT, windowWidth, windowHeight, SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
        VERIFY(Windows, ret);
        VERIFY(Windows, this->width() == width);
        VERIFY(Windows, this->height() == height);
    }
    std::string_view title() const { return m_title; }
    int_t width() const { return m_width; }
    int_t height() const { return m_height; }
    WindowState state() const { return m_state; }

    void toState(WindowState targetState) {
        VERIFY(Windows, state() != WindowState::Destroyed);
        std::underlying_type_t<WindowState> direction = targetState > state() ? 1 : -1;
        while (targetState != state()) {
            WindowState nextState { stx::to_underlying(state()) + direction };
            m_state = nextState;
            // callbacks here...
        }
    }
};

LRESULT CALLBACK mainWindowCallback(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_CREATE) {
        log("WM_CREATE");
        CREATESTRUCT* initParams = (CREATESTRUCT*)lParam;
        WindowImpl* window = (WindowImpl*)initParams->lpCreateParams;
        window->toState(WindowState::Invisible);
        VERIFY(Windows, window);
    }
    WindowImpl* window = (WindowImpl*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if (!window)
        return DefWindowProc(hwnd, msg, wParam, lParam);
    switch (msg) {
    case WM_SIZE: {
        log<Tag::Windows>("WM_SIZE");
        window->updateDimensions();
        return 0;
    }
    case WM_ACTIVATE: {
        log<Tag::Windows>("WM_ACTIVATE {:#b}", wParam);
        bool activate = LOWORD(wParam);
        bool minimized = HIWORD(wParam);
        VERIFY(Windows, !(activate && minimized));
        if (activate)
            window->toState(WindowState::Foreground);
        else
            window->toState(minimized ? WindowState::Invisible : WindowState::Background);
        return 0;
    }
    case WM_DESTROY: {
        log<Tag::Windows>("WM_DESTROY");
        window->toState(WindowState::Destroyed);
        window->destroyImpl();
        return 0;
    }
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

}

#define WINDOW_IMPL plt::windows::WindowImpl
#include "platform/WindowForward.cpp.inc"
#undef WINDOW_IMPL