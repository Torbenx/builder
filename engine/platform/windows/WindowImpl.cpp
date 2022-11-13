#include "WindowImpl.hpp"
#include "core.hpp"
#include "util/Callable.hpp"

namespace plt::windows {

ui::IntVec WindowImpl::calculateWindowSize() {
    RECT rect { 0, 0, currentDimensions.x(), currentDimensions.y() };
    bool ret = AdjustWindowRectEx(&rect, mainWindowStyle, false, mainWindowExStyle);
    WIN_VERIFY(ret);
    return { rect.right - rect.left, rect.bottom - rect.top };
}
void WindowImpl::updateDimensions() {
    RECT clientRect;
    GetClientRect(hwnd(), &clientRect);
    currentDimensions = { clientRect.right, clientRect.bottom };
}
void WindowImpl::initialize() {
    VERIFY(Windows, !hwnd());
    ui::IntVec targetDim = currentDimensions;
    ui::IntVec windowDim = calculateWindowSize();
    if (targetDim.x() == 0)
        windowDim.x() = CW_USEDEFAULT;
    if (targetDim.y() == 0)
        windowDim.y() = CW_USEDEFAULT;

    HWND returned_hwnd = CreateWindowExA(
        mainWindowExStyle, mainWindowClassName, m_title.c_str(), mainWindowStyle,
        CW_USEDEFAULT, CW_USEDEFAULT, windowDim.x(), windowDim.y(),
        NULL, NULL, mainHInstance, (void*)this);
    WIN_VERIFY(returned_hwnd);
    VERIFY(Windows, returned_hwnd == hwnd());

    updateDimensions();
    if (targetDim.x())
        VERIFY(Windows, currentDimensions.x() == targetDim.x());
    if (targetDim.y())
        VERIFY(Windows, currentDimensions.y() == targetDim.y());

    SetWindowLongPtr(hwnd(), GWLP_USERDATA, (uintptr_t)this);
    ShowWindow(hwnd(), SW_SHOW);
}
void WindowImpl::destroy() {
    if (hwnd()) {
        DestroyWindow(hwnd());
    } else {
        incrementalStateChange(evt::WindowState::Destroyed);
        destroyImpl();
    }
}
void WindowImpl::destroyImpl() {
    m_hwnd = nullptr;
    std::string toBeDestroyed = std::move(m_title);
    currentDimensions = {};
}
void WindowImpl::destructor() {
    if (currentState == evt::WindowState::Destroyed || currentState == evt::WindowState::Uninitialized)
        VERIFY(Windows, !hwnd());
    else
        destroy();
}

void WindowImpl::setTitle(std::string_view title) {
    m_title = title;
    if (!hwnd())
        return;
    bool ret = SetWindowText(hwnd(), m_title.c_str());
    WIN_VERIFY(ret);
}
void WindowImpl::setDimensions(ui::IntVec newDim) {
    currentDimensions = newDim;
    if (!hwnd())
        return;
    ui::IntVec windowDim = calculateWindowSize();
    bool ret = SetWindowPos(hwnd(), HWND_TOPMOST, CW_USEDEFAULT, CW_USEDEFAULT, windowDim.x(), windowDim.y(), SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
    WIN_VERIFY(ret);
    VERIFY(Windows, currentDimensions == newDim);
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

void WindowImpl::enterSizeMove() {
    VERIFY(Windows, !inSizeMove);
    inSizeMove = true;
}
void WindowImpl::exitSizeMove() {
    VERIFY(Windows, inSizeMove);
    inSizeMove = false;
}

LRESULT CALLBACK mainWindowCallback(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    using enum evt::WindowState;
    if (msg == WM_CREATE) {
        log("WM_CREATE");
        CREATESTRUCT* initParams = (CREATESTRUCT*)lParam;
        WindowImpl* window = (WindowImpl*)initParams->lpCreateParams;
        VERIFY(Windows, window);
        VERIFY(Windows, window->currentState == Uninitialized);
        window->m_hwnd = hwnd;

        window->currentState = Invisible;
        if (window->surfaceCallback())
            window->surfaceCallback()->onSurfaceCreate(window);

        window->currentState = Uninitialized;
        window->incrementalStateChange(Invisible);
        return 0;
    }
    WindowImpl* window = (WindowImpl*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if (!window)
        return DefWindowProc(hwnd, msg, wParam, lParam);
    switch (msg) {
    case WM_ENTERSIZEMOVE:
        window->enterSizeMove();
        return 0;
    case WM_EXITSIZEMOVE:
        window->exitSizeMove();
        return 0;
    case WM_PAINT: {
        evt::EngineScope::run();
        return 0;
    }
    case WM_WINDOWPOSCHANGED:
        // log<Tag::Windows>("WM_WINDOWPOSCHANGED");
        window->updateDimensions();
        if (window->surfaceCallback())
            window->surfaceCallback()->onSurfaceChange(window);
        window->performResize(window->currentDimensions);
        return 0;
    case WM_ACTIVATE: {
        log<Tag::Windows>("WM_ACTIVATE {:#b}", wParam);
        bool activate = LOWORD(wParam);
        bool minimize = HIWORD(wParam);
        if (activate)
            // minimize indicates whether the window was minimized
            window->incrementalStateChange(Foreground);
        else
            window->incrementalStateChange(minimize ? Invisible : Background);
        return 0;
    }
    case WM_DESTROY: {
        log<Tag::Windows>("WM_DESTROY");
        window->incrementalStateChange(Destroyed);
        window->destroyImpl();
        return 0;
    }
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    VERIFY_NOT_REACHED(Windows);
}

void processEvents() {
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_PAINT)
            break;
        DispatchMessage(&msg);
    }
}

}

namespace plt {

Window Window::construct() {
    auto* window = new windows::WindowImpl;
    return Window(window);
}

}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    using namespace plt::windows;
    mainHInstance = hInstance;

    // register window class
    WNDCLASSEX wc;
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = mainWindowCallback;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = mainWindowClassName;
    wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
    ATOM clazz = RegisterClassEx(&wc);
    WIN_VERIFY(clazz != 0);

    // setup thread local storage
    std::vector<uint32_t> ununsedIndices;
    int_t usedIndices = 0;
    while (usedIndices < plt::TLS_INDICES_COUNT) {
        uint32_t tlsIndex = TlsAlloc();
        if (tlsIndex >= plt::TLS_INDICES_BASE && tlsIndex < plt::TLS_INDICES_BASE + plt::TLS_INDICES_COUNT)
            usedIndices += 1;
        else
            ununsedIndices.push_back(tlsIndex);
        if (tlsIndex >= 63)
            break;
    }
    EXPECT(Windows, usedIndices, plt::TLS_INDICES_COUNT);
    for (uint32_t index : ununsedIndices)
        TlsFree(index);

    evt::EngineScope engine;
    onNoInitialWindow();
    do {
        processEvents();
    } while(evt::EngineScope::run());

    return 0;
}

uint64_t getNanos() {
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    return t.QuadPart * 1'000'000'000 / frequency.QuadPart;
}

void log_print(const std::string& tag, const std::string& msg) {
    std::string str = '[' + tag + "] " + msg + '\n';
    if constexpr (platform::hasMSVCCompilerInterface()) {
        OutputDebugStringA(str.c_str());
    } else {
        printf("%s", str.c_str());
    }
}

#define TO_WINDOW_IMPL plt::windows::windowImpl
#include "platform/WindowForward.cpp.inc"
#undef TO_WINDOW_IMPL