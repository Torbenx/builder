#include "util/fullscreen.hpp"
#include "android/engine.hpp"

FullscreenConfig FullscreenConfig::DEFAULT;
bool engine_fullscreen = false;

FullscreenConfig::FullscreenConfig(){

}

FullscreenConfig::FullscreenConfig([[maybe_unused]] const FullscreenConfig& other){

}

FullscreenConfig::~FullscreenConfig(){

}

void goFullscreen([[maybe_unused]] FullscreenConfig config){
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
