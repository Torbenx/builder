#include "FileExplorer.hpp"
#include <dirent.h>
#include "util/log.hpp"

FileExplorer::FileExplorer(){
    
}

void FileExplorer::onChange(ScreenMetrics metrics){
    for(auto it=fileNameComps.begin();it!=fileNameComps.end();it++){
        (*it)->metrics = metrics;
        (*it)->updateContent();
    }
    hasMetrics = true;
}

void FileExplorer::setPath(std::string path){
    this->path = path;
    scene.builders.clear();
    for(auto it=fileNameComps.begin();it!=fileNameComps.end();it++){
        delete *it;
    }
    fileNameComps.clear();
    clickables.clear();
    const char *s = path.c_str();
    LOGD("path: %s",s);
    DIR *dir =  opendir(s);
    if(dir){
        struct dirent *ent;
        int id = 0;
        while((ent = readdir(dir)) != nullptr){
            TextComponent *text = new TextComponent();
            text->text = std::string(ent->d_name);
            text->textSize = TypedSize::windowsDIP(20);
            text->textColor = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
            text->noBreaks = true;
            scene.layout(*text)
                .wrapContentHeight()
                .alignParentLeft()
                .alignParentRight();
            if(fileNameComps.empty()){
                scene.layout(*text).alignParentTop();
            }else{
                scene.layout(*text).below(*fileNameComps.back());
            }
            fileNameComps.push_back(text);
            makeClickable(*text, id++);
        }
        closedir(dir);
    }else{
        TextComponent *text = new TextComponent();
            text->text = "failed to open directory";
            text->textSize = TypedSize::gl(0.1f);
            text->textColor = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
            text->noBreaks = true;
            scene.layout(*text)
                .wrapContentHeight()
                .alignParentLeft()
                .alignParentRight()
                .alignParentTop();
            fileNameComps.push_back(text);
    }
    if(hasMetrics){
        onChange(scene.metrics);
        scene.build();
    }
}

void FileExplorer::onClick(int id){
    setPath(path+"/"+fileNameComps[id]->text);
}

void FileExplorer::drawFrame(){
    for(auto it=fileNameComps.begin();it!=fileNameComps.end();it++){
        (*it)->draw();
    }
}

FileExplorer::~FileExplorer(){
    
}