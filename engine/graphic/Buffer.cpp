#include "Buffer.hpp"
#include "../util/log.hpp"

template<typename T> Buffer<T>::Buffer(GLint target){
	this->target = target;
}
	
template<typename T> Buffer<T>::~Buffer(){
	glDeleteBuffers(1,&glId);
}
	
template<typename T> bool Buffer<T>::init(){
	if(target!=TE_BUFFER&&!glIsBuffer(glId)){
		glGenBuffers(1,&glId);
		return true;
	}
	return false;
}
	
template<typename T> void Buffer<T>::bufferData(SourceGetter<T*> *data,GLsizeiptr sizeInBytes,GLint usage){
	source = data;
	size = sizeInBytes;
	this->usage = usage;
	if(target!=TE_BUFFER){
		bind();
		glBufferData(target,sizeInBytes,data->getSource(),usage);
	}
}
	
template<typename T> void Buffer<T>::bufferSubData(T *data,GLintptr offset,GLsizeiptr sizeInBytes){
	bind();
	glBufferSubData(target,offset,sizeInBytes,data);
}

template<typename T> void Buffer<T>::bind(){
	glBindBuffer(target,glId);
}
