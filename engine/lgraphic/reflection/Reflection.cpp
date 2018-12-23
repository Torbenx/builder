#include "Reflection.hpp"

Reflection::Reflection(Vec3 reflection,Vec3 position,int width,int height,float factor){
	this->reflection = reflection;
	this->position = position;
	this->factor = factor;
	this->width = width;
	this->height = height;
}
	
void Reflection::getReflectionModel(Mat4x4& model){
	model.setReflection(reflection,position);
}

void Reflection::init(){
	texture.init();
	renderbuffer.init();
	framebuffer.init();
	texture.texture(NULL,width,height,GL_RGBA,GL_UNSIGNED_BYTE);
	renderbuffer.storageDepth16(width, height);
	framebuffer.colorAttachment(texture);
	framebuffer.depthAttachment(renderbuffer);
	framebuffer.viewport(width, height);
}
		
void Reflection::bind(){
	framebuffer.bind();
}
		
void Reflection::unbind(){
	Framebuffer::DEFAULT.bind();
}
