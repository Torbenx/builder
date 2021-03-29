#include "BuildRenderer.hpp"
#include "util/log.hpp"
#include "graphic/Framebuffer.hpp"
#include "fullscreen.hpp"
#include "util/target.hpp"
#include "fullscreen.hpp"
#include <dirent.h>

BuildRenderer::BuildRenderer() : explorer(this), head(this){
    setPath(isAndroid() ? "/data/data/com.tt.builder" : "C:/Users/Torben/Documents/dev/c++");
    #ifdef PLATFORM_ANDROID
        goFullscreen(FullscreenConfig::DEFAULT);
    #endif
    Framebuffer::DEFAULT.clearColor = Vec4(0.1f, 0.1f, 0.1f, 1.0f);
    scene.layout(&head)
        .alignTop()
        .alignLeft()
        .alignRight()
        .height(TypedSize::windowsDIP(200));
    scene.layout(&explorer)
        .below(&head)
        .alignLeft()
        .alignBottom()
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
