#include "PointLighting.hpp"
#include <cmath>
	
PointLighting::PointLighting(Vec3 position,int width,int height,Vec3 color,float _far,float startBrightness,float halfBrightness){
	depthMap = new DepthCubeMap(width,height,_far);
	this->position = position;
	this->color = color;
	this->startBrightness = startBrightness;
	this->halfBrightness = halfBrightness;
}
	
PointLighting::~PointLighting(){
	delete depthMap;
}
	
void PointLighting::init(){
	depthMap->init();
}
	
void PointLighting::draw(Mat4x4 model,Renderable **ptrArray,int count){
	depthMap->draw(model,position,ptrArray,count);
}
