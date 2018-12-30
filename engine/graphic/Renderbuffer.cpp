#include "Renderbuffer.hpp"

Renderbuffer::Renderbuffer() : glId(-1){
    
}
    
Renderbuffer::~Renderbuffer(){
    if(glId.unref()){
        glDeleteRenderbuffers(1,&glId.value);
    }
}
    
bool Renderbuffer::init(){
    if(!glIsRenderbuffer(glId.value)){
        if(glId.refCount != 1){
            LOGW("init on copy");
        }
        glGenRenderbuffers(1,&glId.value);
        return true;
    }
    return false;
}
    
void Renderbuffer::bind(){
    glBindRenderbuffer(GL_RENDERBUFFER,glId.value);
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
