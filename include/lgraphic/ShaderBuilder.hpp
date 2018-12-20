#ifndef HEADER_LGRAPHIC_SHADERBUILDER_H
#define HEADER_LGRAPHIC_SHADERBUILDER_H

#include "../graphic/Shader.hpp"
#include <list>
#include "lighting/AmbientLighting.hpp"
#include "lighting/SpotLighting.hpp"
#include "lighting/PointLighting.hpp"
#include "../util/log.hpp"
#include "LShader.hpp"
#include "reflection/Reflection.hpp"

class ShaderBuilder{
	public:
		std::string vPosition,fColor,colorMulti = "colorMulti",vPositionMulti;
		std::map<void*,std::string> locations;
		int uCount = 0,aCount = 0,vCount = 0,varCount = 0;
		std::ostringstream vMain,fMain,vInit,fInit;
		ShaderBuilder();
		void addLighting(AmbientLighting& l);
		void addLighting(SpotLighting& l);
		void addLighting(PointLighting& l);
		void addReflection(Reflection& r);
		void setFragmentColor(std::string s);
		void setVertexPosition(std::string s);
		void setFragmentColorUniform(std::string s);
		void setFragmentColorUniformTexture(std::string texType,std::string textureUniform,std::string texCoordAttrib);
		void setFragmentColorAttrib(std::string s);
		void setPositionMvpUniform(std::string s);
		void setVertexPositionAttrib(std::string s);
		LShader* build();
		std::string buildVStr();
		std::string buildFStr();
		std::string newUniform(void *obj);
		std::string newAttrib(void *obj);
		std::string newVarying();
		std::string newVar();
		void addVarying(std::string type,std::string name);
		void addVertexUniform(std::string type,std::string name);
		void addFragmentUniform(std::string type,std::string name);
		void addAttrib(std::string type,std::string name);
};

#endif
