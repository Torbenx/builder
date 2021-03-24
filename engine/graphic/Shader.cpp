#include "Shader.hpp"
#include "../util/log.hpp"
#include "../gl/glLog.hpp"

Shader::Shader(std::string vertex,std::string fragment){
    vertexSource = vertex;
    fragmentSource = fragment;
}

Shader::~Shader(){
    glDeleteProgram(glId);
}

bool Shader::init(){
    if(glId == -1){
        const char *vSource = vertexSource.c_str();
        const char *fSource = fragmentSource.c_str();
        //LOGD("%s", vSource);
        //LOGD("%s", fSource);
        glId = glCreateProgram();
        GLuint shader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(shader,1,&vSource,0);
        glCompileShader(shader);
        glAttachShader(glId,shader);
        glDeleteShader(shader);
        shader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(shader,1,&fSource,0);
        glCompileShader(shader);
        glAttachShader(glId,shader);
        glDeleteShader(shader);
        glLinkProgram(glId);
        return true;
    }
    return false;
}

void Shader::use(){
    glUseProgram(glId);
}

GLuint Shader::getAttrib(std::string attrib){
    std::map<std::string,GLint>::iterator it = locations.find(attrib);
    if(it==locations.end()){
        GLuint a = glGetAttribLocation(glId,attrib.c_str());
        locations[attrib] = a;
        return a;
    }else{
        return it->second;
    }
}

GLuint Shader::getAndEnableAttrib(std::string attrib){
    GLuint a = getAttrib(attrib);
    glEnableVertexAttribArray(a);
    return a;
}

GLint Shader::getUniform(std::string uniform){
    std::map<std::string,GLint>::iterator it = locations.find(uniform);
    if(it==locations.end()){
        GLint u = glGetUniformLocation(glId,uniform.c_str());
        locations[uniform] = u;
        return u;
    }else{
        return it->second;
    }
}

void Shader::uniform1f(std::string uniform,float u){
    glUniform1f(getUniform(uniform),u);
}

void Shader::uniform1i(std::string uniform,int u){
    glUniform1i(getUniform(uniform),u);
}

void Shader::uniform2f(std::string uniform,float u1,float u2){
    glUniform2f(getUniform(uniform),u1,u2);
}

void Shader::uniform2f(std::string uniform,float *u){
    glUniform2fv(getUniform(uniform),1,u);
}

void Shader::uniform2i(std::string uniform,int u1,int u2){
    glUniform2i(getUniform(uniform),u1,u2);
}

void Shader::uniform2i(std::string uniform,int *u){
    glUniform2iv(getUniform(uniform),1,u);
}

void Shader::uniform3f(std::string uniform,float u1,float u2,float u3){
    glUniform3f(getUniform(uniform),u1,u2,u3);
}

void Shader::uniform3f(std::string uniform,float *u){
    glUniform3fv(getUniform(uniform),1,u);
}

void Shader::uniform3i(std::string uniform,int u1,int u2,int u3){
    glUniform3i(getUniform(uniform),u1,u2,u3);
}

void Shader::uniform3i(std::string uniform,int *u){
    glUniform3iv(getUniform(uniform),1,u);
}

void Shader::uniform4f(std::string uniform,float u1,float u2,float u3,float u4){
    glUniform4f(getUniform(uniform),u1,u2,u3,u4);
}

void Shader::uniform4f(std::string uniform,float *u){
    glUniform4fv(getUniform(uniform),1,u);
}

void Shader::uniform4i(std::string uniform,int u1,int u2,int u3,int u4){
    glUniform4i(getUniform(uniform),u1,u2,u3,u4);
}

void Shader::uniform4i(std::string uniform,int *u){
    glUniform4iv(getUniform(uniform),1,u);
}

void Shader::uniformMatrix2f(std::string uniform,float *u){
    glUniformMatrix2fv(getUniform(uniform),1,GL_FALSE,u);
}

void Shader::uniformMatrix3f(std::string uniform,float *u){
    glUniformMatrix3fv(getUniform(uniform),1,GL_FALSE,u);
}

void Shader::uniformMatrix4f(std::string uniform,float *u){
    glUniformMatrix4fv(getUniform(uniform),1,GL_FALSE,u);
}

void Shader::uniformVec2(std::string uniform,Vec2 u){
    uniform2f(uniform,u.x,u.y);
}

void Shader::uniformVec3(std::string uniform,Vec3 u){
    uniform3f(uniform,u.x,u.y,u.z);
}

void Shader::uniformVec4(std::string uniform,Vec4 u){
    uniform4f(uniform,u.x,u.y,u.z,u.w);
}

void Shader::uniformMat4x4(std::string uniform,Mat4x4 u){
    uniformMatrix4f(uniform,u.matrix);
}

void Shader::attribPointer(std::string attrib,int size,float *a){
    attribPointer(attrib,size,0,a);
}

void Shader::attribPointer(std::string attrib,int size,int stride,float *a){
    glVertexAttribPointer(getAndEnableAttrib(attrib),size,GL_FLOAT,GL_FALSE,stride,a);
}

void Shader::attribPointer(std::string attrib,int size,Bufferf& a){
    attribPointer(attrib,0,size,0,a);
}

void Shader::attribPointer(std::string attrib,int offset,int size,int stride,Bufferf& a){
    if(a.target==TE_BUFFER){
        glVertexAttribPointer(getAndEnableAttrib(attrib),size,GL_FLOAT,GL_FALSE,stride,a.source+offset);
    }else{
        a.bind();
        glVertexAttribPointer(getAndEnableAttrib(attrib),size,GL_FLOAT,GL_FALSE,stride,(void*)offset);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

void Shader::uniformTexture(std::string uniform,Texture& u,GLuint slot){
    glActiveTexture(slots[slot]);
    u.bind();
    uniform1i(uniform,slot);
}

void Shader::uniformCubeMap(std::string uniform,CubeMap& u,GLuint slot){
    glActiveTexture(slots[slot]);
    u.bind();
    uniform1i(uniform,slot);
}
