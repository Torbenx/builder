#include <stdio.h>
#include "DepthMap.hpp"

const std::string DepthMap::POSITION_ATTRIB = "a_position";
const std::string DepthMap::MVP_MATRIX_UNIFORM = "u_mvpMatrix";

DepthMap::DepthMap(bool divideByW,int width,int height,float _far){
	this->divideByW = divideByW;
	this->width = width;
	this->height = height;
	this->m_far = _far;
	framebuffer.colorAttachment(texture);
	renderbuffer.storageDepth16(width,height);
	framebuffer.depthAttachment(renderbuffer);
	texture.texture(NULL,width,height,GL_RGBA,GL_UNSIGNED_BYTE);
	framebuffer.viewport(width,height);
	framebuffer.setClearColor(1,1,1,1);
	vSource =
		"attribute vec4 a_position;\n"
		"uniform mat4 u_mvpMatrix;\n"
		"varying vec4 v_position;\n"
		"void main(){\n"
		"	v_position = u_mvpMatrix * a_position;\n"
		"	gl_Position = v_position;\n"
		"}";
	std::string f = 
		"varying vec4 v_position;\n"
		"void main(){\n"
		"	float f0 = v_position.z*255.0/";
	char buff[100];
	sprintf(buff,"%f",m_far);
	f.append(buff);
	if(divideByW){
		f.append("/v_position.w");
	}
	f.append(
		";\n"
		"	float f1 = fract(f0);\n"
		"	f0 -= f1;\n"
		"	f1 *= 255.0;\n"
		"	float f2 = fract(f1);\n"
		"	f1 -= f2;\n"
		"	f2 *= 255.0;\n"
		"	float f3 = fract(f2);\n"
		"	f2 -= f3;\n"
		"	f3 *= 255.0;\n"
		"	f3 = ceil(f3);\n"
		"	gl_FragColor = vec4(f0,f1,f2,f3)/255.0;\n"
		//"	gl_FragColor = vec4(1.0,0.0,0.0,1.0);\n"
		"}");
	fSource = f;
	shader = new Shader(vSource,fSource);
}
		
DepthMap::~DepthMap(){
	delete shader;
}
		
void DepthMap::init(){
	texture.init();
	renderbuffer.init();
	framebuffer.init();
	shader->init();
}
		
void DepthMap::draw(MvpMatrix& mvpMatrix,Renderable **ptrArray,int count){
	framebuffer.bind();
	shader->use();
	GLboolean depthTest = glIsEnabled(GL_DEPTH_TEST);
	glEnable(GL_DEPTH_TEST);
	int i;
	std::string mvps[] = {MVP_MATRIX_UNIFORM};
	MvpMatrix *mvpPtr = &mvpMatrix;
	for(i=0;i<count;i++){
		ptrArray[i]->draw(shader,&mvpPtr,POSITION_ATTRIB,"","","","",mvps,1);
	}
	Framebuffer::DEFAULT.bind();
	if(!depthTest)glDisable(GL_DEPTH_TEST);
}
