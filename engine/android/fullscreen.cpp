#include "fullscreen.hpp"
#include "engine.hpp"

FullscreenConfig FullscreenConfig::DEFAULT;

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