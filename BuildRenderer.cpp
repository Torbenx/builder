#include "BuildRenderer.hpp"
#include "util/log.hpp"
#include "graphic/Framebuffer.hpp"
#include "fullscreen.hpp"
#include "util/target.hpp"
#include <dirent.h>

BuildRenderer::BuildRenderer() : explorer(this), head(this){
    
}

void BuildRenderer::create(){
    setPath(isAndroid() ? "/data/data/com.tt.builder" : "C:/");
    Framebuffer::DEFAULT.clearColor = Vec4(0.1f, 0.1f, 0.1f, 1.0f);
    explorer.scene.setParent(Rect::bltr(-1.0f, -1.0f, 0.9f, -0.5f));
    head.scene.setParent(Rect::bltr(0.9f, -1.0f, 1.0f, 1.0f));
}

void BuildRenderer::change(int newWidth, int newHeight){
    Framebuffer::DEFAULT.viewport(newWidth, newHeight);
    explorer.change(newWidth, newHeight);
    head.change(newWidth, newHeight);
}

void BuildRenderer::drawFrame(){
    Framebuffer::DEFAULT.bind();
    explorer.drawFrame();
    head.drawFrame();
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