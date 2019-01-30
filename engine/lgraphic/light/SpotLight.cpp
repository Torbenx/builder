#include "SpotLight.hpp"
#include "../../util/log.hpp"

SpotLight::SpotLight(Vec3 position,Vec3 direction,float hfov,float vfov,int texWidth,int texHeight,Vec3 color,float _far){
    //LOGI("Spot(v: %f ; h: %f ; %d x %d ; %f)",vfov,hfov,texWidth,texHeight,far);
    depthMap = new DepthMap(false,texWidth,texHeight,_far);
    this->position = position;
    this->direction = direction;
    this->up = Vec3(1.0f, 1.0f, -(direction.x + direction.y)/direction.z);
    this->hfov = hfov;
    this->vfov = vfov;
    width = texWidth;
    height = texHeight;
    this->color = color;
    mvpMatrix.projection = Mat4x4::frustumVH(vfov,hfov,1,depthMap->m_far);
}
    
SpotLight::~SpotLight(){
    delete depthMap;
}
    
void SpotLight::init(){
    depthMap->init();
}
    
void SpotLight::draw(Mat4x4 model,Renderable **ptrArray,int count){
    mvpMatrix.model = model;
    mvpMatrix.view = Mat4x4::look(position,direction,up);
    depthMap->draw(mvpMatrix,ptrArray,count);
}
