#ifndef HEADER_FULLSCREEN_H
#define HEADER_FULLSCREEN_H

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

#endif
