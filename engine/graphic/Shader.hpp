#ifndef HEADER_GRAPHIC_SHADER_H
#define HEADER_GRAPHIC_SHADER_H

#include "../gl/glwrapper.hpp"
#include "../util/SourceGetter.hpp"
#include "../math/Vec4.hpp"
#include "../util/Handle.hpp"
#include "Buffer.hpp"
#include "Texture.hpp"
#include "CubeMap.hpp"
#include <map>
#include <string>

class Shader{
    public:
        std::map<std::string,GLint> locations;
        Handle<GLuint> glId;
        std::string vertexSource;
        std::string fragmentSource;
        GLenum slots[32] = {
            GL_TEXTURE0,GL_TEXTURE1,GL_TEXTURE2,GL_TEXTURE3,GL_TEXTURE4,GL_TEXTURE5,
            GL_TEXTURE6,GL_TEXTURE7,GL_TEXTURE8,GL_TEXTURE9,GL_TEXTURE10,GL_TEXTURE11,
            GL_TEXTURE12,GL_TEXTURE13,GL_TEXTURE14,GL_TEXTURE15,GL_TEXTURE16,GL_TEXTURE17,
            GL_TEXTURE18,GL_TEXTURE19,GL_TEXTURE20,GL_TEXTURE21,GL_TEXTURE22,GL_TEXTURE23,
            GL_TEXTURE24,GL_TEXTURE25,GL_TEXTURE26,GL_TEXTURE27,GL_TEXTURE28,GL_TEXTURE29,
            GL_TEXTURE30,GL_TEXTURE31
        };
        Shader(std::string vertex,std::string fragment);
        ~Shader();
        bool init();
        void use();
        GLuint getAttrib(std::string attrib);
        GLuint getAndEnableAttrib(std::string attrib);
        GLint getUniform(std::string uniform);
        
        void uniform1f(std::string uniform,float u);
        void uniform1i(std::string uniform,int u);
        void uniform2f(std::string uniform,float u1,float u2);
        void uniform2f(std::string uniform,float *u);
        void uniform2i(std::string uniform,int u1,int u2);
        void uniform2i(std::string uniform,int *u);
        void uniform3f(std::string uniform,float u1,float u2,float u3);
        void uniform3f(std::string uniform,float *u);
        void uniform3i(std::string uniform,int u1,int u2,int u3);
        void uniform3i(std::string uniform,int *u);
        void uniform4f(std::string uniform,float u1,float u2,float u3,float u4);
        void uniform4f(std::string uniform,float *u);
        void uniform4i(std::string uniform,int u1,int u2,int u3,int u4);
        void uniform4i(std::string uniform,int *u);
        void uniformMatrix2f(std::string uniform,float *u);
        void uniformMatrix3f(std::string uniform,float *u);
        void uniformMatrix4f(std::string uniform,float *u);
        void uniformVec2(std::string uniform,Vec2 u);
        void uniformVec3(std::string uniform,Vec3 u);
        void uniformVec4(std::string uniform,Vec4 u);
        void uniformMat4x4(std::string uniform,Mat4x4 u);
        void uniformTexture(std::string uniform,Texture& u,GLuint slot);
        void uniformCubeMap(std::string uniform,CubeMap& u,GLuint slot);
        
        void attribPointer(std::string attrib,int size,int stride,float *a);
        void attribPointer(std::string attrib,int size,Bufferf& a);
        void attribPointer(std::string attrib,int offset,int size,int stride,Bufferf& a);
};

#endif
