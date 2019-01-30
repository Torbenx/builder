#include "PointLight.hpp"
#include <cmath>
    
PointLight::PointLight(Vec3 position,int width,int height,Vec3 color,float _far,float startBrightness,float halfBrightness){
    depthMap = new DepthCubeMap(width,height,_far);
    this->position = position;
    this->color = color;
    this->startBrightness = startBrightness;
    this->halfBrightness = halfBrightness;
}
    
PointLight::~PointLight(){
    delete depthMap;
}
    
void PointLight::init(){
    depthMap->init();
}
    
void PointLight::draw(Mat4x4 model,Renderable **ptrArray,int count){
    depthMap->draw(model,position,ptrArray,count);
}
