#include "glwrapper.hpp"
#include <string>
#include <iomanip>
#include "../util/log.hpp"
#include "glLog.hpp"
#include <sstream>

std::string glGetErrorStr(){
	int error = glGetError();
	std::stringstream s;
	switch(error){
		case 0x0:
			break;
		case GL_INVALID_ENUM:
			s << "GL_INVALID_ENUM";
			break;
		case GL_INVALID_VALUE:
			s << "GL_INVALID_VALUE";
			break;
		case GL_INVALID_OPERATION:
			s << "GL_INVALID_OPERATION";
			break;
		case GL_OUT_OF_MEMORY:
			s << "GL_OUT_OF_MEMORY";
			break;
		default:
			s << "0x" << std::hex << error;
			break;
	}
	return s.str();
}

void glLogError(std::string fun){
	std::string err = glGetErrorStr();
	if(!err.empty()){
		LOGW("%s: %s",fun.c_str(),err.c_str());
	}
}