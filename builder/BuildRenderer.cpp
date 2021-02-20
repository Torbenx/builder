#include "BuildRenderer.hpp"
#include "util/log.hpp"
#include "graphic/Framebuffer.hpp"
#include "fullscreen.hpp"
#include "util/target.hpp"
#include "fullscreen.hpp"
#include <dirent.h>

BuildRenderer::BuildRenderer() : explorer(this), head(this){
    
}

void BuildRenderer::create(){
    setPath(isAndroid() ? "/data/data/com.tt.builder" : "C:/");
    #ifdef PLATFORM_ANDROID
        goFullscreen(FullscreenConfig::DEFAULT);
    #endif
    Framebuffer::DEFAULT.clearColor = Vec4(0.1f, 0.1f, 0.1f, 1.0f);
    scene.layout(head)
        .alignParentTop()
        .alignParentLeft()
        .alignParentRight()
        .height(TypedSize::windowsDIP(50));
    scene.layout(explorer)
        .below(head)
        .alignParentLeft()
        .alignParentBottom()
        .width(TypedSize::gl(0.5f));
}

void BuildRenderer::change(int newWidth, int newHeight){
    Framebuffer::DEFAULT.viewport(newWidth, newHeight);
    ScreenMetrics metrics;
    int dpi = getDPI();
    metrics.x.size = newWidth;
    metrics.x.dpi = dpi;
    metrics.y.size = newHeight;
    metrics.y.dpi = dpi;
    scene.metrics = metrics;
    scene.build();
    //LOGD("explorer: "); explorer.rect.debug();
    //LOGD("head: "); head.rect.debug();
}

void BuildRenderer::drawFrame(){
    Framebuffer::DEFAULT.bind();
    explorer.draw();
    head.draw();
}

void BuildRenderer::pointer(PointerEvent event){
    explorer.pointer(event);
}

void BuildRenderer::setPath(std::string path){
    files.clear();
    isProject = false;
    DIR *dir =  opendir(path.c_str());
    struct dirent *ent;
    while((ent = readdir(dir)) != nullptr){
        if(!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")){
            continue;
        }
        files.push_back(std::string(ent->d_name));
        if(strcmp(ent->d_name, "script.conf")){
            isProject = true;
        }
    }
    closedir(dir);
    explorer.setPath(path);
    head.setPath(path);
}