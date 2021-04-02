#pragma once

class FullscreenConfig{
public:
    static FullscreenConfig DEFAULT;
    void *impl;
    FullscreenConfig();
    FullscreenConfig(const FullscreenConfig& config);
    ~FullscreenConfig();
};

bool isFullscreen();
void goFullscreen(FullscreenConfig config);
void exitFullscreen();

int getDPI();
