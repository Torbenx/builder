#include "PointLight.hpp"
#include <cmath>

PointLight::PointLight(Vec3 position,int width,int height,Vec3 color,float near,float _far) : depthMap(width,height,_far){
    this->position = position;
    this->color = color;
}

PointLight::~PointLight(){

}

void PointLight::init(){
    depthMap.init();
}

void PointLight::draw(Mat4x4 model, Renderable **ptrArray, int count){
    depthMap.draw(model, position, ptrArray, count);
}
