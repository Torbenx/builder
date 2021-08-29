#pragma once

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES3/gl32.h>

#ifdef __cplusplus
extern "C" {
#endif

void glwrapper_init();

#define GL_MAP_COHERENT_BIT GL_MAP_COHERENT_BIT_EXT
#define GL_MAP_PERSISTENT_BIT GL_MAP_PERSISTENT_BIT_EXT

extern PFNGLBUFFERSTORAGEEXTPROC glBufferStorage;

#ifdef __cplusplus
}
#endif
