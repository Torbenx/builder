#pragma once
#include "util/target.hpp"
#include "util/log.hpp"

#ifdef PLATFORM_WINDOWS
#include "windows/glwrapper.hpp"
#endif

#ifdef PLATFORM_ANDROID
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES3/gl32.h>
#endif

namespace Tag {
struct GL {
    static constexpr char tag[] = "gl";
    static constexpr bool verifyEnabled = true;
};
}

template<VerifyTag tag>
void glGetExpect(GLenum pname, GLint rhs, const char* pnameStr, const char* rhsStr, const char* file, int line, const char* func) requires tag::verifyEnabled {
    GLint lhs;
    glGetIntegerv(pname, &lhs);
    expect<Tag::GL>(lhs, rhs, pnameStr, rhsStr, file, line, func);
}

template<VerifyTag tag>
void glGetExpect(GLenum, GLint, const char*, const char*, const char*, int, const char*) requires(!tag::verifyEnabled) { }

#define GL_GET_EXPECT(pname, rhs) glGetExpect<Tag::GL>(pname, rhs, #pname, #rhs, __FILE__, __LINE__, __func__)
