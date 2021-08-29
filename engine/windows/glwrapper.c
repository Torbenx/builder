//
// Created by Torben Thaysen on 03.02.2016.
//

#include "windows/glwrapper.h"

#define GL_FUNCTION(type, name) type name;
ENUMERATE_GL_FUNCTIONS
#undef GL_FUNCTION

void glwrapper_init(){

#define GL_FUNCTION(type, name) name = (type)wglGetProcAddress(#name);
ENUMERATE_GL_FUNCTIONS
#undef GL_FUNCTION

}
