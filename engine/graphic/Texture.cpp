#include "Texture.hpp"

Texture::Texture(){
    
}

Texture::Texture(GLenum target){
    this->target = target;
}
    
Texture::~Texture(){
    glDeleteTextures(1,&glId);
}
    
bool Texture::init(){
    if(!glIsTexture(glId)){
        glGenTextures(1,&glId);
        return true;
    }
    return false;
}
    
void Texture::bind(){
    glBindTexture(target,glId);
}
    
void Texture::texture(void* data,GLsizei width,GLsizei height,GLenum format,GLenum type){
    source = data;
    this->width = width;
    this->height = height;
    this->format = format;
    this->type = type;
    bind();
    glTexImage2D(target,0,format,width,height,0,format,type,source);
    applyTexParameter();
}
    
void Texture::subTexture(GLvoid *data,GLint xoffset,GLint yoffset,GLsizei width,GLsizei height,GLenum type){
    glTexSubImage2D(target,0,xoffset,yoffset,width,height,format,type,data);
}
    
void Texture::setTexParameter(GLfloat minfilter,GLfloat magfilter,GLfloat wraps,GLfloat wrapt){
    minFilter = minfilter;
    magFilter = magfilter;
    wrapS = wraps;
    wrapT = wrapt;
    applyTexParameter();
}
    
void Texture::applyTexParameter(){
    bind();
    glTexParameterf(target,GL_TEXTURE_MIN_FILTER,minFilter);
    glTexParameterf(target,GL_TEXTURE_MAG_FILTER,magFilter);
    glTexParameterf(target,GL_TEXTURE_WRAP_S,wrapS);
    glTexParameterf(target,GL_TEXTURE_WRAP_T,wrapT);
}
