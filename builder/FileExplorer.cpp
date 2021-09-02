#include "FileExplorer.hpp"
#include "BuildRenderer.hpp"
#include <util/log.hpp>

FileExplorer::FileExplorer(void* renderer)
    : allocator(((BuildRenderer<OpenGL>*)renderer)->allocator) {
    this->renderer = renderer;
}

void FileExplorer::updateRect(ScreenMetrics& metrics, GAllocator& allocator) {
    UiSceneComponent::updateRect(metrics, allocator);
    if (!hasMetrics) {
        hasMetrics = true;
        setPath(path);
    }
}

void FileExplorer::setPath(std::string path) {
    this->path = path;
    scene.clear(allocator);
    scene.layout(&background)
        .alignTop()
        .alignLeft()
        .alignRight()
        .alignBottom();

    for (auto* comp : fileNameComps) {
        delete comp;
    }
    fileNameComps.clear();
    clickables.clear();
    TextComponent* back = new TextComponent();
    back->text = "..";
    back->textSize = TypedSize::windowsDIP(20);
    back->textColor = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
    back->noBreaks = true;
    scene.layout(back)
        .wrapContentHeight()
        .alignLeft()
        .alignRight()
        .alignTop(nullptr, TypedSize::windowsDIP(5));
    fileNameComps.push_back(back);
    makeClickable(*back, 0);

    if (!((BuildRenderer<OpenGL>*)renderer)->files.empty()) {
        int id = 1;
        for (auto it = ((BuildRenderer<OpenGL>*)renderer)->files.begin(); it != ((BuildRenderer<OpenGL>*)renderer)->files.end(); it++) {
            TextComponent* text = new TextComponent();
            text->text = *it;
            text->textSize = TypedSize::windowsDIP(20);
            text->textColor = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
            text->noBreaks = true;
            scene.layout(text)
                .wrapContentWidth()
                .centerHorizontal()
                .wrapContentHeight()
                .below(fileNameComps.back(), TypedSize::windowsDIP(5));
            fileNameComps.push_back(text);
            makeClickable(*text, id++);
        }
    } else {
        TextComponent* text = new TextComponent();
        text->text = "failed to open directory";
        text->textSize = TypedSize::windowsDIP(30);
        text->textColor = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
        text->noBreaks = false;
        scene.layout(text)
            .wrapContentHeight()
            .alignLeft()
            .alignRight()
            .below(back);
        fileNameComps.push_back(text);
    }
    if (hasMetrics) {
        for (auto* comp : fileNameComps) {
            comp->updateContent(scene.metrics, allocator);
        }
        scene.build(allocator);
    }
}

void FileExplorer::onClick(int id) {
    if (id == 0) {
        ((BuildRenderer<OpenGL>*)renderer)->setPath(path.substr(0, path.rfind('/')));
    } else {
        ((BuildRenderer<OpenGL>*)renderer)->setPath(path + "/" + fileNameComps[id]->text);
    }
}

void FileExplorer::draw(Framebuffer& framebuffer, GAllocator& allocator) {
    background.draw(framebuffer, allocator);
    for (auto* comp : fileNameComps) {
        comp->draw(framebuffer, allocator);
    }
}

FileExplorer::~FileExplorer() {
    scene.clear(allocator);
    for (auto* comp : fileNameComps) {
        delete comp;
    }
}
