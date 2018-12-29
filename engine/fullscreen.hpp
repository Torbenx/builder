#ifdef PLATFORM_WINDOWS
#include "windows/fullscreen.hpp"
#endif

#ifdef PLATFORM_ANDROID
#include "android/fullscreen.hpp"
#endif

void goFullscreen(FullscreenConfig config);
void exitFullscreen();