#include "HeadBar.hpp"

HeadBar::HeadBar(void *renderer){
    this->renderer = renderer;
    title.textColor = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
    title.textSize = TypedSize::windowsDIP(40);
    title.noBreaks = true;
}

void HeadBar::setPath(std::string path){
    title.text = path;
    if(hasMetrics){
        updateRect();
    }
}

void HeadBar::updateRect(){
    title.metrics = metrics;
    title.updateContent();
    if(title.textFormat->getWidth() > metrics.x.size){
        scene.layout(title)
            .clear()
            .alignParentTop()
            .alignParentRight()
            .wrapContentWidth()
            .alignParentBottom();
    }else{
        scene.layout(title)
            .clear()
            .alignParentTop()
            .alignParentLeft()
            .wrapContentWidth()
            .alignParentBottom();
    }
    hasMetrics = true;
    UiSceneComponent::updateRect();
}

void HeadBar::draw(){
    title.draw();
}

HeadBar::~HeadBar(){
    
}