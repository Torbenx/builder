#include "util/fullscreen.hpp"
#include "android/engine.hpp"

FullscreenConfig FullscreenConfig::DEFAULT;
bool engine_fullscreen = false;

FullscreenConfig::FullscreenConfig(){

}

FullscreenConfig::FullscreenConfig(const FullscreenConfig& other){

}

FullscreenConfig::~FullscreenConfig(){

}

void goFullscreen(FullscreenConfig config){
    android_goFullscreen();
}

void exitFullscreen(){
    android_exitFullscreen();
}

bool isFullscreen(){
    return engine_fullscreen;
}

int getDPI(){
    return 160;
}
