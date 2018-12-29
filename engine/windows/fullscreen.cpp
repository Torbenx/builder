#include "fullscreen.hpp"
#include "engine.hpp"
#include "../util/log.hpp"
#include <list>

bool equalDEVMODE(DEVMODE first, DEVMODE second){
    return first.dmPelsWidth == second.dmPelsWidth && first.dmPelsHeight == second.dmPelsHeight && first.dmDisplayFrequency == second.dmDisplayFrequency;
}

bool equalDEVMODEIgnoreDisplayFrequency(DEVMODE first, DEVMODE second){
    return first.dmPelsWidth == second.dmPelsWidth && first.dmPelsHeight == second.dmPelsHeight;
}

bool equalDEVMODEIgnoreResolution(DEVMODE first, DEVMODE second){
    return first.dmDisplayFrequency == second.dmDisplayFrequency;
}

bool lessThenDEVMODEResolution(DEVMODE first, DEVMODE second){
    if(first.dmPelsWidth < second.dmPelsWidth)return true;
    if(first.dmPelsWidth > second.dmPelsWidth)return false;
    if(first.dmPelsHeight < second.dmPelsHeight)return true;
    if(first.dmPelsHeight > second.dmPelsHeight)return false;
    return false;
}

bool lessThenDEVMODEDisplayFrequency(DEVMODE first, DEVMODE second){
    if(first.dmDisplayFrequency < second.dmDisplayFrequency)return true;
    return false;
}

FullscreenConfig FullscreenConfig::DEFAULT;

FullscreenConfig::FullscreenConfig(){
    adapterName = "";
    deviceMode.dmSize = 0;
}

FullscreenConfig::FullscreenConfig(std::string adapterName, DEVMODE deviceMode){
    this->adapterName = adapterName;
    this->deviceMode = deviceMode;
}

void fullscreen_init(){
    //delete me
}

std::list<std::string> getDisplayList(){
    int i = 0;
    DISPLAY_DEVICE adapter;
    adapter.cb = sizeof(DISPLAY_DEVICE);
    //std::map<std::string,std::string> monitors;
    std::list<std::string> ret;
    while(EnumDisplayDevices(nullptr, i++, &adapter, 0)){
        LOGD("name: %s, string: %s", adapter.DeviceName, adapter.DeviceString);
        int j = 0;
        DISPLAY_DEVICE monitor;
        monitor.cb = sizeof(DISPLAY_DEVICE);
        while(EnumDisplayDevices(adapter.DeviceName, j++, &monitor, 0)){
            LOGD("    name: %s, string: %s", monitor.DeviceName, monitor.DeviceString);
            //monitors[monitor.DeviceName] = adapter.DeviceName;
            ret.push_back(adapter.DeviceName);
        }
    }
    return ret;
}

FullscreenConfig getFullscreenConfig(std::string adapterName, int width, int height, int refreshRate){
    int i = 0;
    DEVMODE devMode;
    devMode.dmSize = DM_SPECVERSION;
    devMode.dmSize = sizeof(DEVMODE);
    while(EnumDisplaySettings(adapterName.c_str(), i++, &devMode)){
        if(devMode.dmPelsWidth == width && devMode.dmPelsHeight == height && devMode.dmDisplayFrequency == refreshRate){
            LOGD("config found for %s: %ldx%ld@%ld",adapterName.c_str(),devMode.dmPelsWidth,devMode.dmPelsHeight,devMode.dmDisplayFrequency);
            return FullscreenConfig(adapterName, devMode);
        }
    }
    return FullscreenConfig::DEFAULT;
}

int saved_x, saved_y, saved_width, saved_height;
bool engine_fullscreen = false;
FullscreenConfig saved_config;
void goFullscreen(FullscreenConfig config){
    saved_config = config;
    if(!engine_fullscreen){
        engine_fullscreen = true;
        RECT rect;
        GetWindowRect(engine_hwnd, &rect);
        saved_x = rect.left;
        saved_y = rect.top;
        saved_width = rect.right - rect.left;
        saved_height = rect.bottom - rect.top;
    }
    SetWindowLongPtr(engine_hwnd, GWL_EXSTYLE, WS_EX_APPWINDOW);
    SetWindowLongPtr(engine_hwnd, GWL_STYLE, WS_POPUP);
    int width,height;
    if(config.deviceMode.dmSize == 0){
        HMONITOR handle = MonitorFromWindow(engine_hwnd,MONITOR_DEFAULTTOPRIMARY);
        MONITORINFO info;
        info.cbSize = sizeof(MONITORINFO);
        GetMonitorInfo(handle, &info);
        width = info.rcMonitor.right - info.rcMonitor.left;
        height = info.rcMonitor.bottom - info.rcMonitor.top;
    }else{
        width = config.deviceMode.dmPelsWidth;
        height = config.deviceMode.dmPelsHeight;
    }
    
    SetWindowPos(engine_hwnd, HWND_TOPMOST, 0, 0, width, height, 0);
    LOGD("setting %s to: %dx%d@%ld", config.adapterName.c_str(), width, height, config.deviceMode.dmDisplayFrequency);
    long ret = ChangeDisplaySettingsEx(config.adapterName.empty() ? NULL : config.adapterName.c_str(), config.deviceMode.dmSize == 0 ? NULL : &config.deviceMode, NULL, CDS_FULLSCREEN, NULL);
    switch(ret){
        case DISP_CHANGE_SUCCESSFUL: LOGD("DISP_CHANGE_SUCCESSFUL"); break;
        case DISP_CHANGE_BADDUALVIEW: LOGW("DISP_CHANGE_BADDUALVIEW"); break;
        case DISP_CHANGE_BADFLAGS: LOGW("DISP_CHANGE_BADFLAGS"); break;
        case DISP_CHANGE_BADMODE: LOGW("DISP_CHANGE_BADMODE"); break;
        case DISP_CHANGE_BADPARAM: LOGW("DISP_CHANGE_BADPARAM"); break;
        case DISP_CHANGE_FAILED: LOGW("DISP_CHANGE_FAILED"); break;
        case DISP_CHANGE_NOTUPDATED: LOGW("DISP_CHANGE_NOTUPDATED"); break;
        case DISP_CHANGE_RESTART: LOGW("DISP_CHANGE_RESTART"); break;
        default: LOGW("unknown return value: %ld", ret);
    }
    ShowWindow(engine_hwnd, SW_SHOW);
}

void exitFullscreen(){
    if(!engine_fullscreen)return;
    engine_fullscreen = false;
    SetWindowLongPtr(engine_hwnd, GWL_EXSTYLE, WS_EX_CLIENTEDGE);
    SetWindowLongPtr(engine_hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);
    SetWindowPos(engine_hwnd, HWND_NOTOPMOST, saved_x, saved_y, saved_width, saved_height, 0);
    long ret = ChangeDisplaySettingsEx(NULL, NULL, NULL, 0, NULL);
    switch(ret){
        case DISP_CHANGE_SUCCESSFUL: LOGD("DISP_CHANGE_SUCCESSFUL"); break;
        case DISP_CHANGE_BADDUALVIEW: LOGW("DISP_CHANGE_BADDUALVIEW"); break;
        case DISP_CHANGE_BADFLAGS: LOGW("DISP_CHANGE_BADFLAGS"); break;
        case DISP_CHANGE_BADMODE: LOGW("DISP_CHANGE_BADMODE"); break;
        case DISP_CHANGE_BADPARAM: LOGW("DISP_CHANGE_BADPARAM"); break;
        case DISP_CHANGE_FAILED: LOGW("DISP_CHANGE_FAILED"); break;
        case DISP_CHANGE_NOTUPDATED: LOGW("DISP_CHANGE_NOTUPDATED"); break;
        case DISP_CHANGE_RESTART: LOGW("DISP_CHANGE_RESTART"); break;
        default: LOGW("unknown return value: %ld", ret);
    }
    ShowWindow(engine_hwnd, SW_RESTORE);
}

void engine_goForeground(){
    if(engine_fullscreen){
        goFullscreen(saved_config);
    }
}

bool engine_goBackground(){
    if(engine_fullscreen){
        RECT rect;
        GetWindowRect(engine_hwnd, &rect);
        SetWindowPos(engine_hwnd, HWND_NOTOPMOST, rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top, 0);
        long ret = ChangeDisplaySettingsEx(NULL, NULL, NULL, 0, NULL);
        switch(ret){
            case DISP_CHANGE_SUCCESSFUL: LOGD("DISP_CHANGE_SUCCESSFUL"); break;
            case DISP_CHANGE_BADDUALVIEW: LOGW("DISP_CHANGE_BADDUALVIEW"); break;
            case DISP_CHANGE_BADFLAGS: LOGW("DISP_CHANGE_BADFLAGS"); break;
            case DISP_CHANGE_BADMODE: LOGW("DISP_CHANGE_BADMODE"); break;
            case DISP_CHANGE_BADPARAM: LOGW("DISP_CHANGE_BADPARAM"); break;
            case DISP_CHANGE_FAILED: LOGW("DISP_CHANGE_FAILED"); break;
            case DISP_CHANGE_NOTUPDATED: LOGW("DISP_CHANGE_NOTUPDATED"); break;
            case DISP_CHANGE_RESTART: LOGW("DISP_CHANGE_RESTART"); break;
            default: LOGW("unknown return value: %ld", ret);
        }
        ShowWindow(engine_hwnd, SW_MINIMIZE);
        return false;
    }
    return true;
}