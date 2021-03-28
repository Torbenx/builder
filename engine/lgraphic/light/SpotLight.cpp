#include "SpotLight.hpp"
#include "../../util/log.hpp"

SpotLight::SpotLight(Vec3 position,Vec3 direction,float hfov,float vfov,Vec3 hDirection,int texWidth,int texHeight,Vec3 color,float _near,float _far) : depthMap(texWidth,texHeight,_far){
    this->position = position;
    this->direction = direction;
    this->hDirection = hDirection;
    this->hfov = hfov;
    this->vfov = vfov;
    width = texWidth;
    height = texHeight;
    this->color = color;
    mvpMatrix.projection = Mat4x4::frustumVH(vfov,hfov,_near,_far);
}

SpotLight::SpotLight(Vec3 position, Vec3 direction, float fov, int texWidth, int texHeight, Vec3 color, float _near, float _far)
    : SpotLight(position, direction, fov, fov, Vec3::findOrthogonal(direction), texWidth, texHeight, color, _near, _far) {

}

SpotLight::~SpotLight(){

}

void SpotLight::init(){
    depthMap.init();
}

void SpotLight::draw(Mat4x4 model,Renderable **ptrArray,int count){
    mvpMatrix.model = model;
    mvpMatrix.view = Mat4x4::look(position, direction, hDirection);
    depthMap.draw(mvpMatrix, position, ptrArray, count);
}
