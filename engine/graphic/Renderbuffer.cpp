#include "Renderbuffer.hpp"

Renderbuffer::Renderbuffer(){
	
}
	
Renderbuffer::~Renderbuffer(){
	glDeleteRenderbuffers(1,&glId);
}
	
bool Renderbuffer::init(){
	if(!glIsRenderbuffer(glId)){
		glGenRenderbuffers(1,&glId);
		return true;
	}
	return false;
}
	
void Renderbuffer::bind(){
	glBindRenderbuffer(GL_RENDERBUFFER,glId);
}
	
void Renderbuffer::storage(GLenum format,GLsizei width,GLsizei height){
	this->format = format;
	this->width = width;
	this->height = height;
	bind();
	glRenderbufferStorage(GL_RENDERBUFFER,format,width,height);
}
	
void Renderbuffer::storageRGBA5551(GLsizei width,GLsizei height){
	storage(GL_RGB5_A1,width,height);
}
	
void Renderbuffer::storageRGBA4444(GLsizei width,GLsizei height){
	storage(GL_RGBA4,width,height);
}
	
void Renderbuffer::storageDepth16(GLsizei width,GLsizei height){
	storage(GL_DEPTH_COMPONENT16,width,height);
}
	
void Renderbuffer::storageStencil8(GLsizei width,GLsizei height){
	storage(GL_STENCIL_INDEX8,width,height);
}
