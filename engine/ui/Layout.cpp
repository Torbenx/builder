#include "Layout.hpp"
#include "../util/log.hpp"

UiComponentHandle Layout::add(UiComponent *component){
    UiComponentData data;
    data.component = component;
    data.scale = Vec2(0.0f, 0.0f);
    data.translation = Vec2(0.0f, 0.0f);
    components.push_back(data);
    return &components.back();
}


void Layout::setBoundary(UiComponentHandle uc, Boundary boundary){
    Boundary inBoundary = uc->component->getBoundary();
    uc->scale = Vec2(boundary.getWidth(), boundary.getHeight()) / Vec2(inBoundary.getWidth(), inBoundary.getHeight());
    uc->translation = boundary.getTopLeft() - inBoundary.getTopLeft();
}


void Layout::width(UiComponentHandle uc, float width){
    Boundary boundary = uc->component->getBoundary();
    uc->scale.x = width / boundary.getWidth();
}

void Layout::height(UiComponentHandle uc, float height){
    Boundary boundary = uc->component->getBoundary();
    uc->scale.y = height / boundary.getHeight();
}

void Layout::widthPreserveAspect(UiComponentHandle uc, float width){
    Boundary boundary = uc->component->getBoundary();
    uc->scale = Vec2(width / boundary.getWidth());
}

void Layout::heightPreserveAspect(UiComponentHandle uc, float height){
    Boundary boundary = uc->component->getBoundary();
    uc->scale = Vec2(height / boundary.getHeight());
}


void Layout::top(UiComponentHandle uc, float top){
    Boundary boundary = uc->component->getBoundary();
    uc->translation.y = top - uc->scale.y * boundary.getTop();
}

void Layout::bottom(UiComponentHandle uc, float bottom){
    Boundary boundary = uc->component->getBoundary();
    uc->translation.y = bottom - uc->scale.y * boundary.getBottom();
}

void Layout::left(UiComponentHandle uc, float left){
    Boundary boundary = uc->component->getBoundary();
    uc->translation.x = left - uc->scale.x * boundary.getLeft();
}

void Layout::right(UiComponentHandle uc, float right){
    Boundary boundary = uc->component->getBoundary();
    uc->translation.x = right - uc->scale.x * boundary.getRight();
}

void Layout::below(UiComponentHandle above, UiComponentHandle below, float distance){
    Boundary aBoundary = above->component->getBoundary();
    Boundary bBoundary = below->component->getBoundary();
    below->translation.y = aBoundary.getBottom() * above->scale.y + above->translation.y - below->scale.y * bBoundary.getTop() - distance;
}

void Layout::above(UiComponentHandle below, UiComponentHandle above, float distance){
    Boundary bBoundary = below->component->getBoundary();
    Boundary aBoundary = above->component->getBoundary();
    above->translation.y = bBoundary.getTop() * below->scale.y + below->translation.y - above->scale.y * aBoundary.getBottom() + distance;
}


void Layout::setTransform(Mat4x4 transform){
    for(std::list<UiComponentData>::iterator it=components.begin();it!=components.end();it++){
        it->component->setTransform(Mat4x4::translate(Vec3(it->translation, 0.0f)) * Mat4x4::diagonal(Vec3(it->scale, 1.0f)) * transform);
    }
}

void Layout::draw(){
    for(std::list<UiComponentData>::iterator it=components.begin();it!=components.end();it++){
        it->component->draw();
    }
}

Layout::~Layout(){
    for(std::list<UiComponentData>::iterator it=components.begin();it!=components.end();it++){
        delete it->component;
    }
}