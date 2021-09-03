#include "HeadBar.hpp"
#include "BuildRenderer.hpp"

HeadBar::HeadBar(void* renderer)
    : allocator(((BuildRenderer<OpenGL>*)renderer)->allocator) {
    this->renderer = renderer;
    title.textColor = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
    title.textSize = TypedSize::windowsDIP(40);
    title.noBreaks = true;
    subTitle.textColor = Vec4(0.8f, 0.8f, 0.8f, 1.0f);
    subTitle.textSize = TypedSize::windowsDIP(30);
    subTitle.noBreaks = true;
    subTitle.text = "Some Text";
    scene.layout(&title)
        .alignTop()
        .wrapContentHeight()
        .wrapContentWidth()
        .centerHorizontal();
    scene.layout(&subTitle)
        .below(&title)
        .wrapContentHeight()
        .wrapContentWidth()
        .centerHorizontal(&title, 0.75);
    scene.layout(&box)
        .align(&title);
}

void HeadBar::setPath(std::string path) {
    title.text = path;
    title.updateContent(scene.metrics, allocator);
    subTitle.updateContent(scene.metrics, allocator);
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
    scene.build(allocator);
}

void HeadBar::draw(Framebuffer& framebuffer, GAllocator& allocator) {
    box.draw(framebuffer, allocator);
    subTitle.draw(framebuffer, allocator);
    title.draw(framebuffer, allocator);
}

HeadBar::~HeadBar() {
    scene.clear(allocator);
}
