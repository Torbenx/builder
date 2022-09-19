#include "core.hpp"
#include "platform/Window.hpp"
#include "util/log.hpp"
#include "platform/thread_local_storage.hpp"
#include <iostream>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    using namespace plt::windows;
    mainHInstance = hInstance;
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

    if (!RegisterClassEx(&wc)) {
        VERIFY_NOT_REACHED(Windows);
        return 1;
    }

    while (true) {
        uint32_t tlsIndex = TlsAlloc();
        VDUMP(tlsIndex);
        if (tlsIndex >= plt::TLS_INDICES_BASE - 1)
            break;
    }
    for (int_t i = 0; i < plt::TLS_INDICES_COUNT; i++) {
        uint32_t tlsIndex = TlsAlloc();
        VDUMP(tlsIndex);
        VERIFY(Windows, tlsIndex == plt::TLS_INDICES_BASE + (uint32_t)i);
    }

    ::onNoInitialWindow();

    return 0;
}

void log_print(const std::string& tag, const std::string& msg) {
    std::string str = '[' + tag + "] " + msg + '\n';
    if constexpr (platform::hasMSVCCompilerInterface()) {
        OutputDebugStringA(str.c_str());
    } else {
        std::cout << str;
    }
}

namespace plt {
void processEvents() {
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        DispatchMessage(&msg);
}
}