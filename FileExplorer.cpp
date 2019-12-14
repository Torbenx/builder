#include "FileExplorer.hpp"
#include "util/log.hpp"
#include "BuildRenderer.hpp"

FileExplorer::FileExplorer(void *renderer){
    this->renderer = renderer;
}

void FileExplorer::updateRect(){
    for(auto it=fileNameComps.begin();it!=fileNameComps.end();it++){
        (*it)->metrics = metrics;
        (*it)->updateContent();
    }
    hasMetrics = true;
    UiSceneComponent::updateRect();
}

void FileExplorer::setPath(std::string path){
    this->path = path;
    scene.builders.clear();
    for(auto it=fileNameComps.begin();it!=fileNameComps.end();it++){
        delete *it;
    }
    fileNameComps.clear();
    clickables.clear();
    TextComponent *back = new TextComponent();
    back->text = "..";
    back->textSize = TypedSize::windowsDIP(20);
    back->textColor = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
    back->noBreaks = true;
    scene.layout(*back)
        .wrapContentHeight()
        .alignParentLeft()
        .alignParentRight()
        .alignParentTop(TypedSize::windowsDIP(5));
    fileNameComps.push_back(back);
    makeClickable(*back, 0);
    
    if(!((BuildRenderer*)renderer)->files.empty()){
        int id = 1;
        for(auto it=((BuildRenderer*)renderer)->files.begin();it!=((BuildRenderer*)renderer)->files.end();it++){
            TextComponent *text = new TextComponent();
            text->text = *it;
            text->textSize = TypedSize::windowsDIP(20);
            text->textColor = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
            text->noBreaks = true;
            scene.layout(*text)
                .wrapContentHeight()
                .alignParentLeft()
                .alignParentRight()
                .below(*fileNameComps.back(), TypedSize::windowsDIP(5));
            fileNameComps.push_back(text);
            makeClickable(*text, id++);
        }
    }else{
        TextComponent *text = new TextComponent();
            text->text = "failed to open directory";
            text->textSize = TypedSize::windowsDIP(30);
            text->textColor = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
            text->noBreaks = false;
            scene.layout(*text)
                .wrapContentHeight()
                .alignParentLeft()
                .alignParentRight()
                .below(*back);
            fileNameComps.push_back(text);
    }
    if(hasMetrics){
        updateRect();
    }
}

void FileExplorer::onClick(int id){
    if(id == 0){
        ((BuildRenderer*)renderer)->setPath(path.substr(0, path.rfind('/')));
    }else{
        ((BuildRenderer*)renderer)->setPath(path+"/"+fileNameComps[id]->text);
    }
}

void FileExplorer::draw(){
    for(auto it=fileNameComps.begin();it!=fileNameComps.end();it++){
        (*it)->draw();
    }
}

FileExplorer::~FileExplorer(){
    
}