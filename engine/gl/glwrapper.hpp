#ifdef PLATFORM_WINDOWS
#include "../windows/glwrapper.hpp"
#endif

#ifdef PLATFORM_ANDROID
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES3/gl3.h>
#define GL_CLAMP_TO_BORDER 0x812D
#endif
