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

void goFullscreen(FullscreenConfig config);
void exitFullscreen();

#endif