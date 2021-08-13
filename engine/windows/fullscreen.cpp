#include "windows/fullscreen.hpp"
#include "util/log.hpp"
#include "windows/engine.hpp"
#include <list>

bool equalDEVMODE(DEVMODE first, DEVMODE second) {
    return first.dmPelsWidth == second.dmPelsWidth && first.dmPelsHeight == second.dmPelsHeight && first.dmDisplayFrequency == second.dmDisplayFrequency;
}

bool equalDEVMODEIgnoreDisplayFrequency(DEVMODE first, DEVMODE second) {
    return first.dmPelsWidth == second.dmPelsWidth && first.dmPelsHeight == second.dmPelsHeight;
}

bool equalDEVMODEIgnoreResolution(DEVMODE first, DEVMODE second) {
    return first.dmDisplayFrequency == second.dmDisplayFrequency;
}

bool lessThenDEVMODEResolution(DEVMODE first, DEVMODE second) {
    if (first.dmPelsWidth < second.dmPelsWidth)
        return true;
    if (first.dmPelsWidth > second.dmPelsWidth)
        return false;
    if (first.dmPelsHeight < second.dmPelsHeight)
        return true;
    if (first.dmPelsHeight > second.dmPelsHeight)
        return false;
    return false;
}

bool lessThenDEVMODEDisplayFrequency(DEVMODE first, DEVMODE second) {
    if (first.dmDisplayFrequency < second.dmDisplayFrequency)
        return true;
    return false;
}

FullscreenConfig FullscreenConfig::DEFAULT;

FullscreenConfig::FullscreenConfig() {
    impl = new FullscreenConfigImpl();
    ((FullscreenConfigImpl*)impl)->adapterName = "";
    ((FullscreenConfigImpl*)impl)->deviceMode.dmSize = 0;
}

FullscreenConfig::FullscreenConfig(const FullscreenConfig& config) {
    this->impl = new FullscreenConfigImpl();
    *((FullscreenConfigImpl*)impl) = *((FullscreenConfigImpl*)config.impl);
}

FullscreenConfig::~FullscreenConfig() {
    delete (FullscreenConfigImpl*)impl;
}

std::list<std::string> getDisplayList() {
    int i = 0;
    DISPLAY_DEVICE adapter;
    adapter.cb = sizeof(DISPLAY_DEVICE);
    //std::map<std::string,std::string> monitors;
    std::list<std::string> ret;
    while (EnumDisplayDevices(nullptr, i++, &adapter, 0)) {
        log<Tag::Windows>("name: {}, string: {}", adapter.DeviceName, adapter.DeviceString);
        int j = 0;
        DISPLAY_DEVICE monitor;
        monitor.cb = sizeof(DISPLAY_DEVICE);
        while (EnumDisplayDevices(adapter.DeviceName, j++, &monitor, 0)) {
            log<Tag::Windows>("    name: {}, string: {}", monitor.DeviceName, monitor.DeviceString);
            //monitors[monitor.DeviceName] = adapter.DeviceName;
            ret.push_back(adapter.DeviceName);
        }
    }
    return ret;
}

FullscreenConfig getFullscreenConfig(std::string adapterName, int width, int height, int refreshRate) {
    int i = 0;
    DEVMODE devMode;
    devMode.dmSize = DM_SPECVERSION;
    devMode.dmSize = sizeof(DEVMODE);
    while (EnumDisplaySettings(adapterName.c_str(), i++, &devMode)) {
        if ((int)devMode.dmPelsWidth == width && (int)devMode.dmPelsHeight == height && (int)devMode.dmDisplayFrequency == refreshRate) {
            log<Tag::Windows>("config found for {}: {}x{}@{}", adapterName.c_str(), devMode.dmPelsWidth, devMode.dmPelsHeight, devMode.dmDisplayFrequency);
            FullscreenConfig config;
            ((FullscreenConfigImpl*)config.impl)->adapterName = adapterName;
            ((FullscreenConfigImpl*)config.impl)->deviceMode = devMode;
            return config;
        }
    }
    return FullscreenConfig::DEFAULT;
}

void logChangeDisplayResult(long ret) {
    switch (ret) {
    case DISP_CHANGE_SUCCESSFUL: /*log<Tag::Windows>("DISP_CHANGE_SUCCESSFUL");*/
        break;
    case DISP_CHANGE_BADDUALVIEW:
        log<Tag::Windows>("DISP_CHANGE_BADDUALVIEW");
        break;
    case DISP_CHANGE_BADFLAGS:
        log<Tag::Windows>("DISP_CHANGE_BADFLAGS");
        break;
    case DISP_CHANGE_BADMODE:
        log<Tag::Windows>("DISP_CHANGE_BADMODE");
        break;
    case DISP_CHANGE_BADPARAM:
        log<Tag::Windows>("DISP_CHANGE_BADPARAM");
        break;
    case DISP_CHANGE_FAILED:
        log<Tag::Windows>("DISP_CHANGE_FAILED");
        break;
    case DISP_CHANGE_NOTUPDATED:
        log<Tag::Windows>("DISP_CHANGE_NOTUPDATED");
        break;
    case DISP_CHANGE_RESTART:
        log<Tag::Windows>("DISP_CHANGE_RESTART");
        break;
    default:
        log<Tag::Windows>("unknown return value: {}", ret);
    }
}

int saved_x, saved_y, saved_width, saved_height;
bool engine_fullscreen = false;
FullscreenConfigImpl saved_config;
void goFullscreenImpl() {
    SetWindowLongPtr(engine_hwnd, GWL_EXSTYLE, WS_EX_APPWINDOW);
    SetWindowLongPtr(engine_hwnd, GWL_STYLE, WS_POPUP);
    int width, height;
    if (saved_config.deviceMode.dmSize == 0) {
        HMONITOR handle = MonitorFromWindow(engine_hwnd, MONITOR_DEFAULTTOPRIMARY);
        MONITORINFO info;
        info.cbSize = sizeof(MONITORINFO);
        GetMonitorInfo(handle, &info);
        width = info.rcMonitor.right - info.rcMonitor.left;
        height = info.rcMonitor.bottom - info.rcMonitor.top;
    } else {
        width = saved_config.deviceMode.dmPelsWidth;
        height = saved_config.deviceMode.dmPelsHeight;
    }

    SetWindowPos(engine_hwnd, HWND_TOPMOST, 0, 0, width, height, 0);
    log<Tag::Windows>("setting {} to: {}x{}@{}", saved_config.adapterName.c_str(), width, height, saved_config.deviceMode.dmDisplayFrequency);
    long ret;
    if (saved_config.deviceMode.dmSize == 0) {
        ret = ChangeDisplaySettingsEx(NULL, NULL, NULL, CDS_FULLSCREEN, NULL);
    } else {
        ret = ChangeDisplaySettingsEx(saved_config.adapterName.c_str(), &saved_config.deviceMode, NULL, CDS_FULLSCREEN, NULL);
    }
    logChangeDisplayResult(ret);
    ShowWindow(engine_hwnd, SW_SHOW);
}

void goFullscreen(FullscreenConfig config) {
    saved_config = *(FullscreenConfigImpl*)config.impl;
    if (!engine_fullscreen) {
        engine_fullscreen = true;
        RECT rect;
        GetWindowRect(engine_hwnd, &rect);
        saved_x = rect.left;
        saved_y = rect.top;
        saved_width = rect.right - rect.left;
        saved_height = rect.bottom - rect.top;
    }
    if (RendererState::isForeground()) {
        goFullscreenImpl();
    }
}

void exitFullscreenImpl() {
    SetWindowLongPtr(engine_hwnd, GWL_EXSTYLE, WS_EX_CLIENTEDGE);
    SetWindowLongPtr(engine_hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);
    SetWindowPos(engine_hwnd, HWND_NOTOPMOST, saved_x, saved_y, saved_width, saved_height, 0);
    if (saved_config.deviceMode.dmSize != 0) {
        long ret = ChangeDisplaySettingsEx(NULL, NULL, NULL, 0, NULL);
        logChangeDisplayResult(ret);
    }
    ShowWindow(engine_hwnd, SW_RESTORE);
}

void exitFullscreen() {
    if (!engine_fullscreen)
        return;
    engine_fullscreen = false;
    if (RendererState::isForeground()) {
        exitFullscreenImpl();
    }
}

bool isFullscreen() {
    return engine_fullscreen;
}

void engine_goForeground() {
    if (engine_fullscreen) {
        goFullscreenImpl();
    }
}

bool engine_goBackground() {
    if (engine_fullscreen) {
        RECT rect;
        GetWindowRect(engine_hwnd, &rect);
        SetWindowPos(engine_hwnd, HWND_NOTOPMOST, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, 0);
        if (saved_config.deviceMode.dmSize != 0) {
            long ret = ChangeDisplaySettingsEx(NULL, NULL, NULL, 0, NULL);
            logChangeDisplayResult(ret);
        }
        ShowWindow(engine_hwnd, SW_MINIMIZE);
        return false;
    }
    return true;
}

int getDPI() {
    return 96;
}
