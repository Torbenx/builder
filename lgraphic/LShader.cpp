#include "LShader.hpp"

LShader::LShader(std::string vSource,std::string fSource,std::map<void*,std::string> locations) :
Shader(vSource,fSource){
	this->locations = locations;
}
	
void LShader::lighting(AmbientLighting& l){
	uniformVec3(locations[&(l.color)],l.color);
}
		
void LShader::lighting(SpotLighting& l,Mat4x4 model){
	uniformVec3(locations[&(l.color)],l.color);
	uniformMat4x4(locations[&l],l.mvpMatrix.setupMvp(model));
	uniformTexture(locations[l.depthMap],l.depthMap->texture,getTextureSlot(&(l.depthMap->texture)));
}	
		
void LShader::lighting(PointLighting& l,Mat4x4 model){
	uniformVec3(locations[&(l.color)],l.color);
	uniformVec3(locations[&(l.position)],l.position);
	uniformCubeMap(locations[l.depthMap],l.depthMap->texture,getTextureSlot(&(l.depthMap->texture)));
}

void LShader::reflection(Reflection& r){
	uniformTexture(locations[&(r.texture)],r.texture,getTextureSlot(&(r.texture)));
}

unsigned int LShader::getTextureSlot(void *tex){
	if(!textures[tex]){
		textures[tex] = ++textureSlot;
	}
	return textures[tex];
}
