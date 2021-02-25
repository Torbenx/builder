#include "HeadBar.hpp"

HeadBar::HeadBar(void *renderer){
    this->renderer = renderer;
    title.textColor = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
    title.textSize = TypedSize::windowsDIP(40);
    title.noBreaks = true;
    subTitle.textColor = Vec4(0.8f, 0.8f, 0.8f, 1.0f);
    subTitle.textSize = TypedSize::windowsDIP(30);
    subTitle.noBreaks = true;
    subTitle.text = "Some Text";
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
    subTitle.metrics = metrics;
    subTitle.updateContent();
    scene.clear();
    /*if(title.textFormat->getWidth() > metrics.x.size){
        scene.layout(&title)
            .alignParentTop()
            .alignParentRight()
            .wrapContentWidth()
            .alignParentBottom();
    }else{
        scene.layout(&title)
            .alignParentTop()
            .alignParentLeft()
            .wrapContentWidth()
            .alignParentBottom();
    }*/
    scene.layout(&title)
        .alignTop()
        .wrapContentHeight()
        .wrapContentWidth();
    scene.layout(&subTitle)
        .alignBottom(&title)
        .wrapContentHeight()
        .wrapContentWidth()
        .toRightOf(&title, TypedSize::windowsDIP(40));
    scene.layout(&box)
        .alignLeft(&title)
        .alignRight(&subTitle)
        .alignVertical(&title)
        .centerHorizontal();
    hasMetrics = true;
    UiSceneComponent::updateRect();
}

void HeadBar::draw(){
    box.draw();
    subTitle.draw();
    title.draw();
}

HeadBar::~HeadBar(){
    
}