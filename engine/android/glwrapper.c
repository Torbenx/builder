#include "android/glwrapper.h"
#include <EGL/egl.h>

PFNGLBUFFERSTORAGEEXTPROC glBufferStorage;

void glwrapper_init() {
    glBufferStorage = (PFNGLBUFFERSTORAGEEXTPROC)eglGetProcAddress("glBufferStorageEXT");
}
