#include "WindowImpl.hpp"
#include "core.hpp"

namespace plt::windows {

std::pair<int_t, int_t> WindowImpl::calculateWindowSize() {
    RECT rect { 0, 0, (int)width(), (int)height() };
    bool ret = AdjustWindowRectEx(&rect, mainWindowStyle, false, mainWindowExStyle);
    WIN_VERIFY(ret);
    return { rect.right - rect.left, rect.bottom - rect.top };
}
void WindowImpl::updateDimensions() {
    RECT clientRect;
    GetClientRect(hwnd(), &clientRect);
    m_width = clientRect.right;
    m_height = clientRect.bottom;
}
void WindowImpl::initialize() {
    VERIFY(Windows, !hwnd());
    int_t targetWidth = width(), targetHeight = height();
    int windowWidth, windowHeight;
    std::tie(windowWidth, windowHeight) = calculateWindowSize();
    if (width() == 0)
        windowWidth = CW_USEDEFAULT;
    if (height() == 0)
        windowHeight = CW_USEDEFAULT;

    HWND returned_hwnd = CreateWindowExA(
        mainWindowExStyle, mainWindowClassName, m_title.c_str(), mainWindowStyle,
        CW_USEDEFAULT, CW_USEDEFAULT, windowWidth, windowHeight,
        NULL, NULL, mainHInstance, (void*)this);
    WIN_VERIFY(returned_hwnd);
    VERIFY(Windows, returned_hwnd == hwnd());

    updateDimensions();
    if (targetWidth)
        VERIFY(Windows, width() == targetWidth);
    if (targetHeight)
        VERIFY(Windows, height() == targetHeight);

    SetWindowLongPtr(hwnd(), GWLP_USERDATA, (uintptr_t)this);
    ShowWindow(hwnd(), SW_SHOW);
}
void WindowImpl::destroy() {
    toState(WindowState::Destroyed);
    if (hwnd())
        DestroyWindow(hwnd());
    else
        destroyImpl();
}
void WindowImpl::destroyImpl() {
    m_hwnd = nullptr;
    std::string toBeDestroyed = std::move(m_title);
    m_width = 0;
    m_height = 0;
}
void WindowImpl::destructor() {
    if (state() == WindowState::Destroyed || state() == WindowState::Uninitialized)
        VERIFY(Windows, !hwnd());
    else
        destroy();
    stateCallback()->onDetach(this);
}

void WindowImpl::setTitle(std::string_view title) {
    m_title = title;
    if (!hwnd())
        return;
    bool ret = SetWindowText(hwnd(), m_title.c_str());
    WIN_VERIFY(ret);
}
void WindowImpl::setDimensions(int_t width, int_t height) {
    m_width = width;
    m_height = height;
    if (!hwnd())
        return;
    auto [windowWidth, windowHeight] = calculateWindowSize();
    bool ret = SetWindowPos(hwnd(), HWND_TOPMOST, CW_USEDEFAULT, CW_USEDEFAULT, windowWidth, windowHeight, SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
    WIN_VERIFY(ret);
    VERIFY(Windows, this->width() == width);
    VERIFY(Windows, this->height() == height);
}

void WindowImpl::setStateCallback(WindowStateCallback* callback) {
    if (stateCallback())
        stateCallback()->onDetach(this);
    m_stateCallback = callback;
    if (stateCallback())
        stateCallback()->onAttach(this);
}
void WindowImpl::attachSurfaceCallback(WindowSurfaceCallback* callback) {
    VERIFY(Windows, callback);
    VERIFY(Windows, !surfaceCallback());
    m_surfaceCallback = callback;
    surfaceCallback()->onAttach(this);
}
void WindowImpl::detachSurfaceCallback(WindowSurfaceCallback* callback) {
    VERIFY(Windows, callback);
    VERIFY(Windows, surfaceCallback() == callback);
    surfaceCallback()->onDetach(this);
    m_surfaceCallback = nullptr;
}

void WindowImpl::toState(WindowState targetState) {
    VERIFY(Windows, state() != WindowState::Destroyed);
    auto updateState = [&](WindowState newState) {
        WindowState oldState = m_state;
        m_state = newState;
        if (stateCallback())
            stateCallback()->onStageChange(this, oldState, newState);
    };
    if (targetState == WindowState::Destroyed) {
        if (initialized())
            toState(WindowState::Invisible);
        updateState(WindowState::Destroyed);
        return;
    }
    std::underlying_type_t<WindowState> direction = targetState > state() ? 1 : -1;
    while (targetState != state())
        updateState(WindowState { stx::to_underlying(state()) + direction });
}

LRESULT CALLBACK mainWindowCallback(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_CREATE) {
        log("WM_CREATE");
        CREATESTRUCT* initParams = (CREATESTRUCT*)lParam;
        WindowImpl* window = (WindowImpl*)initParams->lpCreateParams;
        VERIFY(Windows, window);
        VERIFY(Windows, window->state() == WindowState::Uninitialized);
        window->m_hwnd = hwnd;
        window->m_state = WindowState::Invisible;
        if (window->surfaceCallback())
            window->surfaceCallback()->onSurfaceCreate(window);
        if (window->stateCallback())
            window->stateCallback()->onStageChange(window, WindowState::Uninitialized, WindowState::Invisible);
        return 0;
    }
    WindowImpl* window = (WindowImpl*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if (!window)
        return DefWindowProc(hwnd, msg, wParam, lParam);
    switch (msg) {
    case WM_SIZE: {
        log<Tag::Windows>("WM_SIZE");
        window->updateDimensions();
        if (window->surfaceCallback())
            window->surfaceCallback()->onSurfaceChange(window);
        return 0;
    }
    case WM_ACTIVATE: {
        log<Tag::Windows>("WM_ACTIVATE {:#b}", wParam);
        bool activate = LOWORD(wParam);
        bool minimize = HIWORD(wParam);
        if (activate)
            // minimize indicates whether the window was minimized
            window->toState(WindowState::Foreground);
        else
            window->toState(minimize ? WindowState::Invisible : WindowState::Background);
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