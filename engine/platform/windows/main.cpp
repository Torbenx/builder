#include "core.hpp"
#include "platform/Window.hpp"
#include "util/log.hpp"
#include "platform/thread_local_storage.hpp"
#include <iostream>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    using namespace plt;
    using namespace plt::windows;
    mainHInstance = hInstance;

    // setup performence counter
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    performenceCounterFrequency = frequency.QuadPart;

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
    while (usedIndices < TLS_INDICES_COUNT) {
        uint32_t tlsIndex = TlsAlloc();
        if (tlsIndex >= TLS_INDICES_BASE && tlsIndex < TLS_INDICES_BASE + TLS_INDICES_COUNT)
            usedIndices += 1;
        else
            ununsedIndices.push_back(tlsIndex);
        if (tlsIndex >= 63)
            break;
    }
    EXPECT(Windows, usedIndices, TLS_INDICES_COUNT);
    for (uint32_t index : ununsedIndices)
        TlsFree(index);

    continueStartup({});

    return 0;
}

uint64_t getNanos() {
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    return t.QuadPart * 1'000'000'000 / plt::windows::performenceCounterFrequency;
}

void log_print(const std::string& tag, const std::string& msg) {
    std::string str = '[' + tag + "] " + msg + '\n';
    if constexpr (platform::hasMSVCCompilerInterface()) {
        OutputDebugStringA(str.c_str());
    } else {
        std::cout << str;
    }
}