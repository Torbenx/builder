#include "CubeMap.hpp"

CubeMap::CubeMap(){
	
}
		
CubeMap::~CubeMap(){
	glDeleteTextures(1,&glId);
}
		
bool CubeMap::init(){
	if(!glIsTexture(glId)){
		glGenTextures(1,&glId);
		applyTexParameter();
		return true;
	}
	return false;
}
		
void CubeMap::bind(){
	glBindTexture(GL_TEXTURE_CUBE_MAP,glId);
}
		
void CubeMap::texturePosX(SourceGetter<void*> *data,GLsizei width,GLsizei height,GLenum format,GLenum type){
	if(data){
		posX = data;
	}else{
		posX = new NullSourceGetter<void*>();
	}
	posXwidth = width;
	posXheight = height;
	posXformat = format;
	posXtype = type;
	texture(GL_TEXTURE_CUBE_MAP_POSITIVE_X,posX,width,height,format,type);
}
		
void CubeMap::textureNegX(SourceGetter<void*> *data,GLsizei width,GLsizei height,GLenum format,GLenum type){
	if(data){
		negX = data;
	}else{
		negX = new NullSourceGetter<void*>();
	}
	negXwidth = width;
	negXheight = height;
	negXformat = format;
	negXtype = type;
	texture(GL_TEXTURE_CUBE_MAP_NEGATIVE_X,negX,width,height,format,type);
}
		
void CubeMap::texturePosY(SourceGetter<void*> *data,GLsizei width,GLsizei height,GLenum format,GLenum type){
	if(data){
		posY = data;
	}else{
		posY = new NullSourceGetter<void*>();
	}
	posYwidth = width;
	posYheight = height;
	posYformat = format;
	posYtype = type;
	texture(GL_TEXTURE_CUBE_MAP_POSITIVE_Y,posY,width,height,format,type);
}
		
void CubeMap::textureNegY(SourceGetter<void*> *data,GLsizei width,GLsizei height,GLenum format,GLenum type){
	if(data){
		negY = data;
	}else{
		negY = new NullSourceGetter<void*>();
	}
	negYwidth = width;
	negYheight = height;
	negYformat = format;
	negYtype = type;
	texture(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,negY,width,height,format,type);
}
		
void CubeMap::texturePosZ(SourceGetter<void*> *data,GLsizei width,GLsizei height,GLenum format,GLenum type){
	if(data){
		posZ = data;
	}else{
		posZ = new NullSourceGetter<void*>();
	}
	posZwidth = width;
	posZheight = height;
	posZformat = format;
	posZtype = type;
	texture(GL_TEXTURE_CUBE_MAP_POSITIVE_Z,posZ,width,height,format,type);
}
		
void CubeMap::textureNegZ(SourceGetter<void*> *data,GLsizei width,GLsizei height,GLenum format,GLenum type){
	if(data){
		negZ = data;
	}else{
		negZ = new NullSourceGetter<void*>();
	}
	negZwidth = width;
	negZheight = height;
	negZformat = format;
	negZtype = type;
	texture(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,negZ,width,height,format,type);
}
		
void CubeMap::applyTexParameter(){
	bind();
	glTexParameterf(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_MIN_FILTER,minFilter);
	glTexParameterf(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_MAG_FILTER,magFilter);
	glTexParameterf(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_WRAP_S,wrapS);
	glTexParameterf(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_WRAP_T,wrapT);
}
		
void CubeMap::setTexParameter(GLfloat minfilter,GLfloat magfilter,GLfloat wraps,GLfloat wrapt){
	minFilter = minfilter;
	magFilter = magfilter;
	wrapS = wraps;
	wrapT = wrapt;
	applyTexParameter();
}
		
void CubeMap::texture(GLenum target,SourceGetter<void*> *data,GLsizei width,GLsizei height,GLenum format,GLenum type){
	bind();
	glTexImage2D(target,0,format,width,height,0,format,type,data->getSource());
}
