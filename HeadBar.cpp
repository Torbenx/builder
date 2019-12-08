#include "HeadBar.hpp"

HeadBar::HeadBar(void *renderer){
    this->renderer = renderer;
    title.textColor = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
    title.textSize = TypedSize::gl(0.05f);
    title.noBreaks = true;
    scene.layout(title)
        .alignParentTop()
        .alignParentLeft()
        .wrapContent();
}

void HeadBar::setPath(std::string path){
    title.text = path;
    if(hasMetrics){
        title.updateContent();
        scene.build();
    }
}

void HeadBar::onChange(ScreenMetrics metrics){
    title.metrics = metrics;
    title.updateContent();
    hasMetrics = true;
}

void HeadBar::drawFrame(){
    title.draw();
}

HeadBar::~HeadBar(){
    
}