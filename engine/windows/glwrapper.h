//
// Created by Torben Thaysen on 03.02.2016.
//

#pragma once

#include <GL/gl.h>
#include <GL/glcorearb.h>
#include <GL/wglext.h>
#include <Windows.h>

#ifdef __cplusplus
extern "C" {
#endif

void glwrapper_init();

#define ENUMERATE_GL_FUNCTIONS                                                             \
                                                                                           \
    GL_FUNCTION(PFNWGLCREATECONTEXTATTRIBSARBPROC, wglCreateContextAttribsARB)             \
    GL_FUNCTION(PFNGLDEBUGMESSAGECALLBACKARBPROC, glDebugMessageCallbackARB)               \
                                                                                           \
    GL_FUNCTION(PFNWGLSWAPINTERVALEXTPROC, wglSwapIntervalEXT)                             \
                                                                                           \
    GL_FUNCTION(PFNWGLDXOPENDEVICENVPROC, wglDXOpenDeviceNV)                               \
    GL_FUNCTION(PFNWGLDXCLOSEDEVICENVPROC, wglDXCloseDeviceNV)                             \
    GL_FUNCTION(PFNWGLDXREGISTEROBJECTNVPROC, wglDXRegisterObjectNV)                       \
    GL_FUNCTION(PFNWGLDXUNREGISTEROBJECTNVPROC, wglDXUnregisterObjectNV)                   \
    GL_FUNCTION(PFNWGLDXLOCKOBJECTSNVPROC, wglDXLockObjectsNV)                             \
    GL_FUNCTION(PFNWGLDXUNLOCKOBJECTSNVPROC, wglDXUnlockObjectsNV)                         \
                                                                                           \
    GL_FUNCTION(PFNGLACTIVETEXTUREPROC, glActiveTexture)                                   \
    GL_FUNCTION(PFNGLATTACHSHADERPROC, glAttachShader)                                     \
    GL_FUNCTION(PFNGLBEGINCONDITIONALRENDERPROC, glBeginConditionalRender)                 \
    GL_FUNCTION(PFNGLBEGINQUERYPROC, glBeginQuery)                                         \
    GL_FUNCTION(PFNGLBEGINTRANSFORMFEEDBACKPROC, glBeginTransformFeedback)                 \
    GL_FUNCTION(PFNGLBINDATTRIBLOCATIONPROC, glBindAttribLocation)                         \
    GL_FUNCTION(PFNGLBINDBUFFERPROC, glBindBuffer)                                         \
    GL_FUNCTION(PFNGLBINDFRAGDATALOCATIONPROC, glBindFragDataLocation)                     \
    GL_FUNCTION(PFNGLBINDFRAMEBUFFERPROC, glBindFramebuffer)                               \
    GL_FUNCTION(PFNGLBINDRENDERBUFFERPROC, glBindRenderbuffer)                             \
    GL_FUNCTION(PFNGLBLENDCOLORPROC, glBlendColor)                                         \
    GL_FUNCTION(PFNGLBLENDEQUATIONPROC, glBlendEquation)                                   \
    GL_FUNCTION(PFNGLBLENDEQUATIONSEPARATEPROC, glBlendEquationSeparate)                   \
    GL_FUNCTION(PFNGLBLENDFUNCSEPARATEPROC, glBlendFuncSeparate)                           \
    GL_FUNCTION(PFNGLBLENDFUNCSEPARATEIPROC, glBlendFuncSeparatei)                         \
    GL_FUNCTION(PFNGLBLITFRAMEBUFFERPROC, glBlitFramebuffer)                               \
    GL_FUNCTION(PFNGLBUFFERDATAPROC, glBufferData)                                         \
    GL_FUNCTION(PFNGLBUFFERSUBDATAPROC, glBufferSubData)                                   \
    GL_FUNCTION(PFNGLCHECKFRAMEBUFFERSTATUSPROC, glCheckFramebufferStatus)                 \
    GL_FUNCTION(PFNGLCLAMPCOLORPROC, glClampColor)                                         \
    GL_FUNCTION(PFNGLCOMPILESHADERPROC, glCompileShader)                                   \
    GL_FUNCTION(PFNGLCOMPRESSEDTEXIMAGE1DPROC, glCompressedTexImage1D)                     \
    GL_FUNCTION(PFNGLCOMPRESSEDTEXIMAGE2DPROC, glCompressedTexImage2D)                     \
    GL_FUNCTION(PFNGLCOMPRESSEDTEXIMAGE3DPROC, glCompressedTexImage3D)                     \
    GL_FUNCTION(PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC, glCompressedTexSubImage1D)               \
    GL_FUNCTION(PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC, glCompressedTexSubImage2D)               \
    GL_FUNCTION(PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC, glCompressedTexSubImage3D)               \
    GL_FUNCTION(PFNGLCOPYTEXSUBIMAGE3DPROC, glCopyTexSubImage3D)                           \
    GL_FUNCTION(PFNGLCREATEPROGRAMPROC, glCreateProgram)                                   \
    GL_FUNCTION(PFNGLCREATESHADERPROC, glCreateShader)                                     \
    GL_FUNCTION(PFNGLDELETEBUFFERSPROC, glDeleteBuffers)                                   \
    GL_FUNCTION(PFNGLDELETEFRAMEBUFFERSPROC, glDeleteFramebuffers)                         \
    GL_FUNCTION(PFNGLDELETEPROGRAMPROC, glDeleteProgram)                                   \
    GL_FUNCTION(PFNGLDELETEQUERIESPROC, glDeleteQueries)                                   \
    GL_FUNCTION(PFNGLDELETERENDERBUFFERSPROC, glDeleteRenderbuffers)                       \
    GL_FUNCTION(PFNGLDELETESHADERPROC, glDeleteShader)                                     \
    GL_FUNCTION(PFNGLDETACHSHADERPROC, glDetachShader)                                     \
    GL_FUNCTION(PFNGLDISABLEIPROC, glDisablei)                                             \
    GL_FUNCTION(PFNGLDISABLEVERTEXATTRIBARRAYPROC, glDisableVertexAttribArray)             \
    GL_FUNCTION(PFNGLDRAWARRAYSINSTANCEDPROC, glDrawArraysInstanced)                       \
    GL_FUNCTION(PFNGLDRAWBUFFERSPROC, glDrawBuffers)                                       \
    GL_FUNCTION(PFNGLDRAWELEMENTSINSTANCEDPROC, glDrawElementsInstanced)                   \
    GL_FUNCTION(PFNGLDRAWRANGEELEMENTSPROC, glDrawRangeElements)                           \
    GL_FUNCTION(PFNGLENABLEIPROC, glEnablei)                                               \
    GL_FUNCTION(PFNGLENABLEVERTEXATTRIBARRAYPROC, glEnableVertexAttribArray)               \
    GL_FUNCTION(PFNGLENDCONDITIONALRENDERPROC, glEndConditionalRender)                     \
    GL_FUNCTION(PFNGLENDQUERYPROC, glEndQuery)                                             \
    GL_FUNCTION(PFNGLENDTRANSFORMFEEDBACKPROC, glEndTransformFeedback)                     \
    GL_FUNCTION(PFNGLFRAMEBUFFERRENDERBUFFERPROC, glFramebufferRenderbuffer)               \
    GL_FUNCTION(PFNGLFRAMEBUFFERTEXTUREPROC, glFramebufferTexture)                         \
    GL_FUNCTION(PFNGLFRAMEBUFFERTEXTURE1DPROC, glFramebufferTexture1D)                     \
    GL_FUNCTION(PFNGLFRAMEBUFFERTEXTURE2DPROC, glFramebufferTexture2D)                     \
    GL_FUNCTION(PFNGLFRAMEBUFFERTEXTURE3DPROC, glFramebufferTexture3D)                     \
    GL_FUNCTION(PFNGLFRAMEBUFFERTEXTURELAYERPROC, glFramebufferTextureLayer)               \
    GL_FUNCTION(PFNGLGENBUFFERSPROC, glGenBuffers)                                         \
    GL_FUNCTION(PFNGLGENERATEMIPMAPPROC, glGenerateMipmap)                                 \
    GL_FUNCTION(PFNGLGENFRAMEBUFFERSPROC, glGenFramebuffers)                               \
    GL_FUNCTION(PFNGLGENQUERIESPROC, glGenQueries)                                         \
    GL_FUNCTION(PFNGLGENRENDERBUFFERSPROC, glGenRenderbuffers)                             \
    GL_FUNCTION(PFNGLGETACTIVEATTRIBPROC, glGetActiveAttrib)                               \
    GL_FUNCTION(PFNGLGETACTIVEUNIFORMPROC, glGetActiveUniform)                             \
    GL_FUNCTION(PFNGLGETATTACHEDSHADERSPROC, glGetAttachedShaders)                         \
    GL_FUNCTION(PFNGLGETATTRIBLOCATIONPROC, glGetAttribLocation)                           \
    GL_FUNCTION(PFNGLGETBUFFERSUBDATAPROC, glGetBufferSubData)                             \
    GL_FUNCTION(PFNGLGETCOMPRESSEDTEXIMAGEPROC, glGetCompressedTexImage)                   \
    GL_FUNCTION(PFNGLGETFRAGDATALOCATIONPROC, glGetFragDataLocation)                       \
    GL_FUNCTION(PFNGLGETPROGRAMINFOLOGPROC, glGetProgramInfoLog)                           \
    GL_FUNCTION(PFNGLGETSHADERINFOLOGPROC, glGetShaderInfoLog)                             \
    GL_FUNCTION(PFNGLGETSHADERSOURCEPROC, glGetShaderSource)                               \
    GL_FUNCTION(PFNGLGETSTRINGIPROC, glGetStringi)                                         \
    GL_FUNCTION(PFNGLGETTRANSFORMFEEDBACKVARYINGPROC, glGetTransformFeedbackVarying)       \
    GL_FUNCTION(PFNGLGETUNIFORMLOCATIONPROC, glGetUniformLocation)                         \
    GL_FUNCTION(PFNGLISBUFFERPROC, glIsBuffer)                                             \
    GL_FUNCTION(PFNGLISFRAMEBUFFERPROC, glIsFramebuffer)                                   \
    GL_FUNCTION(PFNGLISPROGRAMPROC, glIsProgram)                                           \
    GL_FUNCTION(PFNGLISQUERYPROC, glIsQuery)                                               \
    GL_FUNCTION(PFNGLISRENDERBUFFERPROC, glIsRenderbuffer)                                 \
    GL_FUNCTION(PFNGLISSHADERPROC, glIsShader)                                             \
    GL_FUNCTION(PFNGLLINKPROGRAMPROC, glLinkProgram)                                       \
    GL_FUNCTION(PFNGLMAPBUFFERPROC, glMapBuffer)                                           \
    GL_FUNCTION(PFNGLMINSAMPLESHADINGPROC, glMinSampleShading)                             \
    GL_FUNCTION(PFNGLMULTIDRAWARRAYSPROC, glMultiDrawArrays)                               \
    GL_FUNCTION(PFNGLMULTIDRAWELEMENTSPROC, glMultiDrawElements)                           \
    GL_FUNCTION(PFNGLPRIMITIVERESTARTINDEXPROC, glPrimitiveRestartIndex)                   \
    GL_FUNCTION(PFNGLRENDERBUFFERSTORAGEPROC, glRenderbufferStorage)                       \
    GL_FUNCTION(PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC, glRenderbufferStorageMultisample) \
    GL_FUNCTION(PFNGLSAMPLECOVERAGEPROC, glSampleCoverage)                                 \
    GL_FUNCTION(PFNGLSHADERSOURCEPROC, glShaderSource)                                     \
    GL_FUNCTION(PFNGLSTENCILFUNCSEPARATEPROC, glStencilFuncSeparate)                       \
    GL_FUNCTION(PFNGLSTENCILMASKSEPARATEPROC, glStencilMaskSeparate)                       \
    GL_FUNCTION(PFNGLSTENCILOPSEPARATEPROC, glStencilOpSeparate)                           \
    GL_FUNCTION(PFNGLTEXBUFFERPROC, glTexBuffer)                                           \
    GL_FUNCTION(PFNGLTEXIMAGE3DPROC, glTexImage3D)                                         \
    GL_FUNCTION(PFNGLTEXSUBIMAGE3DPROC, glTexSubImage3D)                                   \
    GL_FUNCTION(PFNGLTRANSFORMFEEDBACKVARYINGSPROC, glTransformFeedbackVaryings)           \
    GL_FUNCTION(PFNGLUNMAPBUFFERPROC, glUnmapBuffer)                                       \
    GL_FUNCTION(PFNGLUSEPROGRAMPROC, glUseProgram)                                         \
    GL_FUNCTION(PFNGLVALIDATEPROGRAMPROC, glValidateProgram)                               \
    GL_FUNCTION(PFNGLVERTEXATTRIBDIVISORPROC, glVertexAttribDivisor)                       \
    GL_FUNCTION(PFNGLVERTEXATTRIBIPOINTERPROC, glVertexAttribIPointer)                     \
    GL_FUNCTION(PFNGLVERTEXATTRIBPOINTERPROC, glVertexAttribPointer)                       \
    GL_FUNCTION(PFNGLUNIFORM1FPROC, glUniform1f)                                           \
    GL_FUNCTION(PFNGLUNIFORM1FVPROC, glUniform1fv)                                         \
    GL_FUNCTION(PFNGLUNIFORM1IPROC, glUniform1i)                                           \
    GL_FUNCTION(PFNGLUNIFORM1IVPROC, glUniform1iv)                                         \
    GL_FUNCTION(PFNGLUNIFORM2FPROC, glUniform2f)                                           \
    GL_FUNCTION(PFNGLUNIFORM2FVPROC, glUniform2fv)                                         \
    GL_FUNCTION(PFNGLUNIFORM2IPROC, glUniform2i)                                           \
    GL_FUNCTION(PFNGLUNIFORM2IVPROC, glUniform2iv)                                         \
    GL_FUNCTION(PFNGLUNIFORM3FPROC, glUniform3f)                                           \
    GL_FUNCTION(PFNGLUNIFORM3FVPROC, glUniform3fv)                                         \
    GL_FUNCTION(PFNGLUNIFORM3IPROC, glUniform3i)                                           \
    GL_FUNCTION(PFNGLUNIFORM3IVPROC, glUniform3iv)                                         \
    GL_FUNCTION(PFNGLUNIFORM4FPROC, glUniform4f)                                           \
    GL_FUNCTION(PFNGLUNIFORM4FVPROC, glUniform4fv)                                         \
    GL_FUNCTION(PFNGLUNIFORM4IPROC, glUniform4i)                                           \
    GL_FUNCTION(PFNGLUNIFORM4IVPROC, glUniform4iv)                                         \
    GL_FUNCTION(PFNGLUNIFORMMATRIX2FVPROC, glUniformMatrix2fv)                             \
    GL_FUNCTION(PFNGLUNIFORMMATRIX3FVPROC, glUniformMatrix3fv)                             \
    GL_FUNCTION(PFNGLUNIFORMMATRIX4FVPROC, glUniformMatrix4fv)                             \
    GL_FUNCTION(PFNGLVERTEXATTRIB1DPROC, glVertexAttrib1d)                                 \
    GL_FUNCTION(PFNGLVERTEXATTRIB1DVPROC, glVertexAttrib1dv)                               \
    GL_FUNCTION(PFNGLVERTEXATTRIB1FPROC, glVertexAttrib1f)                                 \
    GL_FUNCTION(PFNGLVERTEXATTRIB1FVPROC, glVertexAttrib1fv)                               \
    GL_FUNCTION(PFNGLVERTEXATTRIB1SPROC, glVertexAttrib1s)                                 \
    GL_FUNCTION(PFNGLVERTEXATTRIB1SVPROC, glVertexAttrib1sv)                               \
    GL_FUNCTION(PFNGLVERTEXATTRIB2DPROC, glVertexAttrib2d)                                 \
    GL_FUNCTION(PFNGLVERTEXATTRIB2DVPROC, glVertexAttrib2dv)                               \
    GL_FUNCTION(PFNGLVERTEXATTRIB2FPROC, glVertexAttrib2f)                                 \
    GL_FUNCTION(PFNGLVERTEXATTRIB2FVPROC, glVertexAttrib2fv)                               \
    GL_FUNCTION(PFNGLVERTEXATTRIB2SPROC, glVertexAttrib2s)                                 \
    GL_FUNCTION(PFNGLVERTEXATTRIB2SVPROC, glVertexAttrib2sv)                               \
    GL_FUNCTION(PFNGLVERTEXATTRIB3DPROC, glVertexAttrib3d)                                 \
    GL_FUNCTION(PFNGLVERTEXATTRIB3DVPROC, glVertexAttrib3dv)                               \
    GL_FUNCTION(PFNGLVERTEXATTRIB3FPROC, glVertexAttrib3f)                                 \
    GL_FUNCTION(PFNGLVERTEXATTRIB3FVPROC, glVertexAttrib3fv)                               \
    GL_FUNCTION(PFNGLVERTEXATTRIB3SPROC, glVertexAttrib3s)                                 \
    GL_FUNCTION(PFNGLVERTEXATTRIB3SVPROC, glVertexAttrib3sv)                               \
    GL_FUNCTION(PFNGLVERTEXATTRIB4NBVPROC, glVertexAttrib4Nbv)                             \
    GL_FUNCTION(PFNGLVERTEXATTRIB4NIVPROC, glVertexAttrib4Niv)                             \
    GL_FUNCTION(PFNGLVERTEXATTRIB4NSVPROC, glVertexAttrib4Nsv)                             \
    GL_FUNCTION(PFNGLVERTEXATTRIB4NUBPROC, glVertexAttrib4Nub)                             \
    GL_FUNCTION(PFNGLVERTEXATTRIB4NUBVPROC, glVertexAttrib4Nubv)                           \
    GL_FUNCTION(PFNGLVERTEXATTRIB4NUIVPROC, glVertexAttrib4Nuiv)                           \
    GL_FUNCTION(PFNGLVERTEXATTRIB4NUSVPROC, glVertexAttrib4Nusv)                           \
    GL_FUNCTION(PFNGLVERTEXATTRIB4BVPROC, glVertexAttrib4bv)                               \
    GL_FUNCTION(PFNGLVERTEXATTRIB4DPROC, glVertexAttrib4d)                                 \
    GL_FUNCTION(PFNGLVERTEXATTRIB4DVPROC, glVertexAttrib4dv)                               \
    GL_FUNCTION(PFNGLVERTEXATTRIB4FPROC, glVertexAttrib4f)                                 \
    GL_FUNCTION(PFNGLVERTEXATTRIB4FVPROC, glVertexAttrib4fv)                               \
    GL_FUNCTION(PFNGLVERTEXATTRIB4IVPROC, glVertexAttrib4iv)                               \
    GL_FUNCTION(PFNGLVERTEXATTRIB4SPROC, glVertexAttrib4s)                                 \
    GL_FUNCTION(PFNGLVERTEXATTRIB4SVPROC, glVertexAttrib4sv)                               \
    GL_FUNCTION(PFNGLVERTEXATTRIB4UBVPROC, glVertexAttrib4ubv)                             \
    GL_FUNCTION(PFNGLVERTEXATTRIB4UIVPROC, glVertexAttrib4uiv)                             \
    GL_FUNCTION(PFNGLVERTEXATTRIB4USVPROC, glVertexAttrib4usv)                             \
    GL_FUNCTION(PFNGLGENVERTEXARRAYSPROC, glGenVertexArrays)                               \
    GL_FUNCTION(PFNGLBINDVERTEXARRAYPROC, glBindVertexArray)                               \
    GL_FUNCTION(PFNGLGETSHADERIVPROC, glGetShaderiv)                                       \
                                                                                           \
    GL_FUNCTION(PFNGLBUFFERSTORAGEPROC, glBufferStorage)                                   \
    GL_FUNCTION(PFNGLMAPBUFFERRANGEPROC, glMapBufferRange)                                 \
    GL_FUNCTION(PFNGLBINDBUFFERRANGEPROC, glBindBufferRange)

#define GL_FUNCTION(type, name) extern type name;
ENUMERATE_GL_FUNCTIONS;
#undef GL_FUNCTION

#ifdef __cplusplus
}
#endif
