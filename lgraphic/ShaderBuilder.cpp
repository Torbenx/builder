#include <stdio.h>
#include "ShaderBuilder.hpp"
#include "string.h"

ShaderBuilder::ShaderBuilder(){
	
}

void ShaderBuilder::addLighting(AmbientLighting& l){
	std::string colorUniform = newUniform(&(l.color));
	addFragmentUniform("vec3", colorUniform);
	fMain<<colorMulti<<" = max("<<colorMulti<<","<<colorUniform<<");\n";
}
		
void ShaderBuilder::addLighting(SpotLighting& l){
	std::string mvpUniform = newUniform(&l);
	std::string textureUniform = newUniform(l.depthMap);
	std::string colorUniform = newUniform(&(l.color));
	std::string positionVarying = newVarying();
	addVertexUniform("mat4",mvpUniform);
	addFragmentUniform("sampler2D",textureUniform);
	addFragmentUniform("vec3",colorUniform);
	addVarying("vec4",positionVarying);
	std::string m_far = newVar();
	std::string depth = newVar();
	vMain<<positionVarying<<" = "<<mvpUniform<<" * "<<vPosition<<";\n";
	char buff[100];
	sprintf(buff,"%f",l.depthMap->m_far);
	fMain<<"const float "<<m_far<<" = "<<buff<<";\n";
	fMain<<"float "<<depth<<" = dot(vec4("<<m_far<<","<<m_far<<"/255.0,"<<m_far<<"/255.0/255.0,"<<m_far<<"/255.0/255.0/255.0),texture2D("<<textureUniform<<",("<<positionVarying<<".xy/"<<positionVarying<<".w+1.0)/2.0));\n";
	fMain<<"if("<<positionVarying<<".z>0.0&&dot("<<positionVarying<<".xy,"<<positionVarying<<".xy)<=1.0&&"<<positionVarying<<".z<="<<depth<<"*1.01){"<<colorMulti<<" = max("<<colorMulti<<","<<colorUniform<<");}\n";
}
		
void ShaderBuilder::addLighting(PointLighting& l){
	std::string textureUniform = newUniform(l.depthMap);
	std::string colorUniform = newUniform(&(l.color));
	std::string lightPosUniform = newUniform(&(l.position));
	std::string positionVarying = newVarying();
	addFragmentUniform("samplerCube",textureUniform);
	addFragmentUniform("vec3",colorUniform);
	addFragmentUniform("vec3",lightPosUniform);
	addVarying("vec4",positionVarying);
	vMain<<positionVarying<<" = "<<vPosition<<";\n";
	std::string _far = newVar();
	std::string depth = newVar();
	std::string pos = newVar();
	std::string distance = newVar();
	char buff[100];
	sprintf(buff,"%f",l.depthMap->m_far);
	fMain<<"vec3 "<<pos<<" = "<<positionVarying<<".xyz/"<<positionVarying<<".w;\n";
	fMain<<"const float "<<_far<<" = "<<buff<<";\n";
	float invOff0 = 1.0f / l.startBrightness;
	sprintf(buff,"%f",(1.0f/l.halfBrightness-invOff0)*4.0f/(l.depthMap->m_far*l.depthMap->m_far));
	fMain<<"float "<<depth<<" = dot(vec4("<<_far<<","<<_far<<"/255.0,"<<_far<<"/255.0/255.0,"<<_far<<"/255.0/255.0/255.0),textureCube("<<textureUniform<<","<<pos<<"-"<<lightPosUniform<<"));\n";
	fMain<<"if("<<depth<<"==0.0){"<<depth<<" = "<<_far<<";}\n";
	fMain<<"float "<<distance<<" = distance("<<pos<<","<<lightPosUniform<<");\n";
	fMain<<"if("<<distance<<"<="<<depth<<"*1.01){"<<colorMulti<<" = max("<<colorMulti<<","<<colorUniform<<"/("<<invOff0<<"+"<<buff<<"*"<<distance<<"*"<<distance<<"));}\n";
}

void ShaderBuilder::addReflection(Reflection& r){
	std::string textureUniform = newUniform(&(r.texture));
	std::string positionVarying = newVarying();
	addVarying("vec4", positionVarying);
	addFragmentUniform("sampler2D",textureUniform);
	vMain<<positionVarying<<" = gl_Position;\n";
	char buff[100];
	sprintf(buff,"%f",r.factor);
	fMain<<fColor<<" += "<<buff<<"*texture2D("<<textureUniform<<",("<<positionVarying<<".xy/"<<positionVarying<<".w+1.0)/2.0);\n";
}
		
void ShaderBuilder::setFragmentColor(std::string s){
	fColor = newVar();
	fMain<<"vec4 "<<fColor<<" = "<<s<<";\n";
}
		
void ShaderBuilder::setVertexPosition(std::string s){
	vPosition = s;
}
		
void ShaderBuilder::setFragmentColorUniform(std::string s){
	addFragmentUniform("vec4",s);
	setFragmentColor(s);
}
		
void ShaderBuilder::setFragmentColorUniformTexture(std::string texType,std::string textureUniform,std::string texCoordAttrib){
	std::string texCoordVarying = newVarying();
	std::string color = newVar();
	addVarying("vec2",texCoordVarying);
	addAttrib("vec2",texCoordAttrib);
	std::string s = "sampler";
	s.append(texType);
	addFragmentUniform(s,textureUniform);
	vMain<<texCoordVarying<<" = "<<texCoordAttrib<<";\n";
	fMain<<"vec4 "<<color<<" = texture"<<texType<<"("<<textureUniform<<","<<texCoordVarying<<");\n";
	setFragmentColor(color);
}
		
void ShaderBuilder::setFragmentColorAttrib(std::string s){
	std::string varying = newVarying();
	addVarying("vec4",varying);
	addAttrib("vec4",s);
	vMain<<varying<<" = "<<s<<";\n";
	setFragmentColor(varying);
}
		
void ShaderBuilder::setPositionMvpUniform(std::string s){
	addVertexUniform("mat4",s);
	vPositionMulti = s;
}
		
void ShaderBuilder::setVertexPositionAttrib(std::string s){
	addAttrib("vec4",s);
	setVertexPosition(s);
}
		
LShader* ShaderBuilder::build(){
	return new LShader(buildVStr(),buildFStr(),locations);
}
		
std::string ShaderBuilder::buildVStr(){
	std::ostringstream v;
	v<<vInit.str()<<"void main(){\ngl_Position = ";
	if(!vPositionMulti.empty()){
		v<<vPositionMulti<<" * ";
	}
	v<<vPosition<<";\n"<<vMain.str()<<"}";
	return v.str();
}
		
std::string ShaderBuilder::buildFStr(){
	std::ostringstream f;
	f<<fInit.str()<<"void main(){\nvec3 "<<colorMulti<<" = vec3(0.0);\n";
	f<<fMain.str()<<"gl_FragColor = vec4("<<colorMulti<<", 1.0) * "<<fColor<<";\n}";
	return f.str();
}
		
std::string ShaderBuilder::newUniform(void *obj){
	std::ostringstream s;
	s<<"u_"<<(uCount++);
	locations[obj] = s.str();
	return locations[obj];
}
		
std::string ShaderBuilder::newAttrib(void *obj){
	std::ostringstream s;
	s<<"a_"<<(aCount++);
	locations[obj] = s.str();
	return locations[obj];
}
		
std::string ShaderBuilder::newVarying(){
	std::ostringstream s;
	s<<"v_"<<(vCount++);
	return s.str();
}
		
std::string ShaderBuilder::newVar(){
	std::ostringstream s;
	s<<"var"<<(varCount++);
	return s.str();
}

void ShaderBuilder::addVarying(std::string type,std::string name){
	vInit<<"varying "<<type<<" "<<name<<";\n";
	fInit<<"varying "<<type<<" "<<name<<";\n";
}
		
void ShaderBuilder::addVertexUniform(std::string type,std::string name){
	vInit<<"uniform "<<type<<" "<<name<<";\n";
}
		
void ShaderBuilder::addFragmentUniform(std::string type,std::string name){
	fInit<<"uniform "<<type<<" "<<name<<";\n";
}
		
void ShaderBuilder::addAttrib(std::string type,std::string name){
	vInit<<"attribute "<<type<<" "<<name<<";\n";
}
