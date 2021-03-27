#include "SpotLight.hpp"
#include "../../util/log.hpp"

SpotLight::SpotLight(Vec3 position,Vec3 direction,float hfov,float vfov,int texWidth,int texHeight,Vec3 color,float _near,float _far) : depthMap(texWidth,texHeight,_far){
    this->position = position;
    this->direction = direction;
    this->up = Vec3::findOrthogonal(direction);
    this->hfov = hfov;
    this->vfov = vfov;
    width = texWidth;
    height = texHeight;
    this->color = color;
    mvpMatrix.projection = Mat4x4::frustumVH(vfov,hfov,_near,_far);
}

SpotLight::~SpotLight(){

}

void SpotLight::init(){
    depthMap.init();
}

void SpotLight::draw(Mat4x4 model,Renderable **ptrArray,int count){
    mvpMatrix.model = model;
    mvpMatrix.view = Mat4x4::look(position, direction, up);
    depthMap.draw(mvpMatrix, position, ptrArray, count);
}
