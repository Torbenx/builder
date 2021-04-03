#include "GL/Shader.hpp"
#include "util/log.hpp"

std::string ShaderBuilder::buildVertexSource(){
    std::ostringstream s;
    s << GLSL_VERSION_HEADER << vInit.str() << "void main(){\n" << vMain.str() << "}";
    return s.str();
}

std::string ShaderBuilder::buildFragmentSource(){
    std::ostringstream s;
    s << GLSL_VERSION_HEADER << fInit.str() << "out vec4 gl_FragColor;\n" << "void main(){\n" << fMain.str() << "}";
    return s.str();
}

void ShaderBuilder::newContext(){
    contextCount += 1;
}

std::string ShaderBuilder::uniqueName(std::string name){
    std::ostringstream s;
    s << name << contextCount;
    return s.str();
}

Shader::LocationHandle ShaderBuilder::addAttrib(std::string type, std::string name, std::string qualifiers){
    vInit << qualifiers << "in " << type << " " << name << ";\n";
    GLuint handle = locations.size();
    locations.push_back(Shader::Location(name, false));
    return { handle };
}

Shader::LocationHandle ShaderBuilder::addUniform(std::string type, std::string name, std::string qualifiers, bool vertex){
    (vertex ? vInit : fInit) << qualifiers << "uniform " << type << " " << name << ";\n";
    GLuint handle = locations.size();
    locations.push_back(Shader::Location(name, true));
    return { handle };
}

void ShaderBuilder::addVarying(std::string type, std::string name, std::string qualifiers){
    vInit << qualifiers << "out " << type << " " << name << ";\n";
    fInit << qualifiers << "in " << type << " " << name << ";\n";
}

void ShaderBuilder::requireExtension(std::string extension){
    vInit << "#extension " << extension << " : require\n";
    fInit << "#extension " << extension << " : require\n";
}

int ShaderBuilder::nextTextureSlot(){
#ifdef VALIDATE_GL
    GLint maxTextures;
    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxTextures);
    GL_VERIFY(textureSlot < maxTextures);
#endif
    return textureSlot++;
}

void ShaderBuilder::disableFragmentShader(){
    fragmentShaderDisabled = true;
}

void Shader::initalize(ShaderBuilder& builder){
    GL_VERIFY(!glIsProgram(glId));
    glId = glCreateProgram();

    auto compileShader = [&](bool vertex, const char *source){
        GLuint shader = glCreateShader(vertex ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER);
        glShaderSource(shader, 1, &source, 0);
        glCompileShader(shader);
#ifdef VALIDATE_GL
        GLint success = GL_FALSE;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if(success == GL_FALSE){
            GLint length = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
            GLchar message[length];
            glGetShaderInfoLog(shader, length, &length, message);
            LOGW("\n"
            "--------------------\n"
            "%s Shader failed to compile:\n"
            "-------SOURCE-------\n%s\n"
            "-------ERROR--------\n%s\n", vertex ? "Vertex" : "Fragment", source, message);
        }else
#endif
            glAttachShader(glId, shader);

        glDeleteShader(shader);
    };

    std::string vSource = builder.buildVertexSource();
    compileShader(true, vSource.c_str());

    if(!builder.fragmentShaderDisabled){
        std::string fSource = builder.buildFragmentSource();
        compileShader(false, fSource.c_str());
    }

    glLinkProgram(glId);

    locations.resize(builder.locations.size());
    for(int i=0;i<builder.locations.size();i++){
        if(builder.locations[i].isUniform){
            locations[i] = glGetUniformLocation(glId, builder.locations[i].name.c_str());
        }else{
            locations[i] = glGetAttribLocation(glId, builder.locations[i].name.c_str());
        }
    }
}

Shader::Shader(Shader&& other) : glId(other.glId), locations(std::move(other.locations)) {
    other.glId = -1;
}

Shader::~Shader(){
    if(glId != -1){
        glDeleteProgram(glId);
    }
}

void Shader::use(){
    GL_VERIFY(glIsProgram(glId));
    glUseProgram(glId);
}

GLuint Shader::getAttrib(Shader::LocationHandle attrib){
    return locations[attrib.handle];
}

GLuint Shader::getAndEnableAttrib(Shader::LocationHandle attrib){
    GLuint a = getAttrib(attrib);
    glEnableVertexAttribArray(a);
    return a;
}

GLint Shader::getUniform(Shader::LocationHandle uniform){
    return locations[uniform.handle];
}

void Shader::uniform1f(Shader::LocationHandle uniform,float u){
    glUniform1f(getUniform(uniform),u);
}

void Shader::uniform1i(Shader::LocationHandle uniform,int u){
    glUniform1i(getUniform(uniform),u);
}

void Shader::uniform2f(Shader::LocationHandle uniform,float u1,float u2){
    glUniform2f(getUniform(uniform),u1,u2);
}

void Shader::uniform2f(Shader::LocationHandle uniform,float *u){
    glUniform2fv(getUniform(uniform),1,u);
}

void Shader::uniform2i(Shader::LocationHandle uniform,int u1,int u2){
    glUniform2i(getUniform(uniform),u1,u2);
}

void Shader::uniform2i(Shader::LocationHandle uniform,int *u){
    glUniform2iv(getUniform(uniform),1,u);
}

void Shader::uniform3f(Shader::LocationHandle uniform,float u1,float u2,float u3){
    glUniform3f(getUniform(uniform),u1,u2,u3);
}

void Shader::uniform3f(Shader::LocationHandle uniform,float *u){
    glUniform3fv(getUniform(uniform),1,u);
}

void Shader::uniform3i(Shader::LocationHandle uniform,int u1,int u2,int u3){
    glUniform3i(getUniform(uniform),u1,u2,u3);
}

void Shader::uniform3i(Shader::LocationHandle uniform,int *u){
    glUniform3iv(getUniform(uniform),1,u);
}

void Shader::uniform4f(Shader::LocationHandle uniform,float u1,float u2,float u3,float u4){
    glUniform4f(getUniform(uniform),u1,u2,u3,u4);
}

void Shader::uniform4f(Shader::LocationHandle uniform,float *u){
    glUniform4fv(getUniform(uniform),1,u);
}

void Shader::uniform4i(Shader::LocationHandle uniform,int u1,int u2,int u3,int u4){
    glUniform4i(getUniform(uniform),u1,u2,u3,u4);
}

void Shader::uniform4i(Shader::LocationHandle uniform,int *u){
    glUniform4iv(getUniform(uniform),1,u);
}

void Shader::uniformMatrix2f(Shader::LocationHandle uniform,float *u){
    glUniformMatrix2fv(getUniform(uniform),1,GL_FALSE,u);
}

void Shader::uniformMatrix3f(Shader::LocationHandle uniform,float *u){
    glUniformMatrix3fv(getUniform(uniform),1,GL_FALSE,u);
}

void Shader::uniformMatrix4f(Shader::LocationHandle uniform,float *u){
    glUniformMatrix4fv(getUniform(uniform),1,GL_FALSE,u);
}

void Shader::uniformVec2(Shader::LocationHandle uniform,Vec2 u){
    uniform2f(uniform,u.x,u.y);
}

void Shader::uniformVec3(Shader::LocationHandle uniform,Vec3 u){
    uniform3f(uniform,u.x,u.y,u.z);
}

void Shader::uniformVec4(Shader::LocationHandle uniform,Vec4 u){
    uniform4f(uniform,u.x,u.y,u.z,u.w);
}

void Shader::uniformMat4x4(Shader::LocationHandle uniform, Mat4x4& u){
    uniformMatrix4f(uniform, u.matrix);
}

void Shader::attribPointer(Shader::LocationHandle attrib, int size, void* a){
    attribPointer(attrib, 0, size, 0, a);
}

void Shader::attribPointer(Shader::LocationHandle attrib, int offset, int size, int stride, void* a){
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glVertexAttribPointer(getAndEnableAttrib(attrib), size, GL_FLOAT, GL_FALSE, stride, a);
}

void Shader::attribPointer(Shader::LocationHandle attrib, int size, Buffer<float>& a){
    attribPointer(attrib, 0, size, 0, a);
}

void Shader::attribPointer(Shader::LocationHandle attrib, int offset, int size, int stride, Buffer<float>& a){
    a.bind(GL_ARRAY_BUFFER);
    glVertexAttribPointer(getAndEnableAttrib(attrib), size, GL_FLOAT, GL_FALSE, stride, a.data());
}

void Shader::uniformTexture(Shader::LocationHandle uniform, Texture& u, GLuint slot){
    glActiveTexture(GL_TEXTURE0 + slot);
    u.bind();
    uniform1i(uniform, slot);
}

void Shader::uniformCubeMap(Shader::LocationHandle uniform, CubeMap& u, GLuint slot){
    glActiveTexture(GL_TEXTURE0 + slot);
    u.bind();
    uniform1i(uniform, slot);
}
