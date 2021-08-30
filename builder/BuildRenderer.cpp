#include "BuildRenderer.hpp"
#include <util/log.hpp>
#include <GL/Framebuffer.hpp>
#include <util/target.hpp>
#include <util/fullscreen.hpp>
#include <dirent.h>

template<Gapi G>
BuildRenderer<G>::BuildRenderer() : explorer(this), head(this){
    setPath(isAndroid() ? "/data/data/com.tt.builder" : "C:/Users/Torben/Documents/dev/c++");
    Framebuffer::DEFAULT.clearColor(0.1f, 0.1f, 0.1f, 1.0f);
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

template<Gapi G>
void BuildRenderer<G>::change(int_t newWidth, int_t newHeight){
    Framebuffer::DEFAULT.viewport(newWidth, newHeight);
    ScreenMetrics metrics;
    int_t dpi = getDPI();
    metrics.x.size = newWidth;
    metrics.x.dpi = dpi;
    metrics.y.size = newHeight;
    metrics.y.dpi = dpi;
    scene.metrics = metrics;
    scene.build();
}

template<Gapi G>
void BuildRenderer<G>::drawFrame(){
    Framebuffer::DEFAULT.clear();
    explorer.draw(Framebuffer::DEFAULT, scene.allocator);
    head.draw(Framebuffer::DEFAULT, scene.allocator);
}

template<Gapi G>
void BuildRenderer<G>::pointer(PointerEvent event){
    explorer.pointer(event);
}

template<Gapi G>
void BuildRenderer<G>::setPath(std::string path){
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

INSTANTIATE_G_API_CLASS(BuildRenderer);
