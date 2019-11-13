//#include "Python.h"
#include "BuildRenderer.hpp"
#include "util/log.hpp"
#include <dirent.h>
#include "graphic/Framebuffer.hpp"

BuildRenderer::BuildRenderer(){}

void BuildRenderer::create(){
    DIR *dir =  opendir("/");
    if(dir){
        struct dirent *ent;
        while((ent = readdir(dir)) != nullptr){
            TextComponent *text = new TextComponent();
            text->text = std::string(ent->d_name);
            text->textSize = TypedSize::gl(0.1f);
            text->textColor = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
            text->noBreaks = true;
            projectChooser.layout(*text)
                .wrapContentHeight()
                .alignParentLeft()
                .alignParentRight();
            if(fileNameComps.empty()){
                projectChooser.layout(*text).alignParentTop();
            }else{
                projectChooser.layout(*text).below(*fileNameComps.back());
            }
            fileNameComps.push_back(text);
        }
        closedir(dir);
    }else{
        TextComponent *text = new TextComponent();
            text->text = "failed to open directory";
            text->textSize = TypedSize::gl(0.1f);
            text->textColor = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
            text->noBreaks = true;
            projectChooser.layout(*text)
                .wrapContentHeight()
                .alignParentLeft()
                .alignParentRight()
                .alignParentTop();
            fileNameComps.push_back(text);
    }
}

void BuildRenderer::change(int newWidth, int newHeight){
    Framebuffer::DEFAULT.viewport(newWidth, newHeight);
    ScreenMetrics metrics;
    metrics.x.size = newWidth;
    metrics.y.size = newHeight;
    for(auto it=fileNameComps.begin();it!=fileNameComps.end();it++){
        (*it)->metrics = metrics;
        (*it)->updateContent();
    }
    projectChooser.metrics = metrics;
    projectChooser.build();
    for(auto it=fileNameComps.begin();it!=fileNameComps.end();it++){
        (*it)->rect.debug();
    }
}

void BuildRenderer::drawFrame(){
    Framebuffer::DEFAULT.bind();
    for(auto it=fileNameComps.begin();it!=fileNameComps.end();it++){
        (*it)->draw();
    }
}