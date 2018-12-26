#include "SpotLighting.hpp"
#include "../../util/log.hpp"

SpotLighting::SpotLighting(Vec3 position,Vec3 direction,Vec3 up,float hfov,float vfov,int texWidth,int texHeight,Vec3 color,float _far){
    //LOGI("Spot(v: %f ; h: %f ; %d x %d ; %f)",vfov,hfov,texWidth,texHeight,far);
    depthMap = new DepthMap(false,texWidth,texHeight,_far);
    this->position = position;
    this->direction = direction;
    this->up = up;
    this->hfov = hfov;
    this->vfov = vfov;
    width = texWidth;
    height = texHeight;
    this->color = color;
    mvpMatrix.projection.setFrustumVH(vfov,hfov,1,depthMap->m_far);
}
    
SpotLighting::~SpotLighting(){
    delete depthMap;
}
    
void SpotLighting::init(){
    depthMap->init();
}
    
void SpotLighting::draw(Mat4x4 model,Renderable **ptrArray,int count){
    mvpMatrix.model = model;
    mvpMatrix.view.setLookAt(position,position+direction,up);
    depthMap->draw(mvpMatrix,ptrArray,count);
}
