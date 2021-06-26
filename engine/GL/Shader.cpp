#include "GL/Shader.hpp"
#include "util/log.hpp"
#include "util/target.hpp"

const std::string Shader::FRAGMENT_COLOR = "_fragmentColor";

ShaderBuilder::ShaderBuilder() {
    if (usingGLES()) {
        vExt << "#version 310 es\n";
        fExt << "#version 310 es\n";
        fInit << "precision mediump float;\n";
        fInit << "precision lowp sampler2DShadow;\n";
        fInit << "precision lowp samplerCubeShadow;\n";
    } else {
        vExt << "#version 150\n";
        fExt << "#version 150\n";
    }
}

std::string ShaderBuilder::buildVertexSource() {
    std::ostringstream s;
    s << vExt.str() << vInit.str() << "void main(){\n"
      << vMain.str() << "}";
    return s.str();
}

std::string ShaderBuilder::buildFragmentSource() {
    std::ostringstream s;
    s << fExt.str() << fInit.str() << "out vec4 " << Shader::FRAGMENT_COLOR << ";\n"
      << "void main(){\n"
      << fMain.str() << "}";
    return s.str();
}

void ShaderBuilder::newContext() {
    contextCount += 1;
}

std::string ShaderBuilder::uniqueName(std::string name) {
    std::ostringstream s;
    s << name << contextCount;
    return s.str();
}

Shader::LocationHandle ShaderBuilder::addAttrib(std::string type, std::string name, std::string qualifiers) {
    vInit << qualifiers << "in " << type << " " << name << ";\n";
    GLuint handle = locations.size();
    locations.push_back(Shader::Location(name, false));
    return { handle };
}

Shader::LocationHandle ShaderBuilder::addUniform(std::string type, std::string name, std::string qualifiers, bool vertex) {
    (vertex ? vInit : fInit) << qualifiers << "uniform " << type << " " << name << ";\n";
    GLuint handle = locations.size();
    locations.push_back(Shader::Location(name, true));
    return { handle };
}

void ShaderBuilder::addVarying(std::string type, std::string name, std::string qualifiers) {
    vInit << qualifiers << "out " << type << " " << name << ";\n";
    fInit << qualifiers << "in " << type << " " << name << ";\n";
}

void ShaderBuilder::requireExtension(std::string extension) {
    vExt << "#extension " << extension << " : require\n";
    fExt << "#extension " << extension << " : require\n";
}

int ShaderBuilder::nextTextureSlot() {
#ifdef VALIDATE_GL
    GLint maxTextures;
    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxTextures);
    GL_VERIFY(textureSlot < maxTextures);
#endif
    return textureSlot++;
}

void ShaderBuilder::disableFragmentShader() {
    fragmentShaderDisabled = true;
}

void Shader::initialize(ShaderBuilder& builder) {
    GL_VERIFY(!glIsProgram(glId));
    glId = glCreateProgram();
    LOGD("initalize glId=%d", glId);

    auto printSource = [](std::string source) {
        std::istringstream stream(source);
        const int bufSize = 256;
        char line[bufSize];
        int lineNumber = 1;
        while (stream.getline(line, bufSize)) {
            LOGW("%2d: %s", lineNumber++, line);
        }
    };

    auto compileShader = [&](bool vertex, const char* source) {
        GLuint shader = glCreateShader(vertex ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER);
        glShaderSource(shader, 1, &source, 0);
        glCompileShader(shader);
#ifdef VALIDATE_GL
        GLint success = GL_FALSE;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (success == GL_FALSE) {
            GLint length = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
            std::vector<char> message(length);
            glGetShaderInfoLog(shader, length, &length, message.data());
            LOGW("\n"
                 "%s Shader failed to compile:\n"
                 "--------------SOURCE--------------\n",
                vertex ? "Vertex" : "Fragment");
            printSource(std::string(source));
            LOGW("--------------ERROR---------------\n%s\n", message.data());
        } else
#endif
        {
            glAttachShader(glId, shader);
        }
        glDeleteShader(shader);
    };

    std::string vSource = builder.buildVertexSource();
    compileShader(true, vSource.c_str());

    if (usingGLES() || !builder.fragmentShaderDisabled) {
        std::string fSource = builder.buildFragmentSource();
        compileShader(false, fSource.c_str());
    }

    glLinkProgram(glId);

    locations.resize(builder.locations.size());
    for (int i = 0; i < builder.locations.size(); i++) {
        if (builder.locations[i].isUniform) {
            locations[i] = glGetUniformLocation(glId, builder.locations[i].name.c_str());
        } else {
            locations[i] = glGetAttribLocation(glId, builder.locations[i].name.c_str());
        }
    }
}

void Shader::release() {
    LOGD("release glId=%d", glId);
    if (glId != 0) {
        glDeleteProgram(glId);
    }
}

bool Shader::isValid() const {
    return glId != 0;
}

void Shader::use() const {
    GL_VERIFY(glIsProgram(glId));
    glUseProgram(glId);
}

GLuint Shader::getAttrib(Shader::LocationHandle attrib) const {
    return locations[attrib.handle];
}

GLuint Shader::getAndEnableAttrib(Shader::LocationHandle attrib) const {
    GLuint a = getAttrib(attrib);
    glEnableVertexAttribArray(a);
    return a;
}

GLint Shader::getUniform(Shader::LocationHandle uniform) const {
    return locations[uniform.handle];
}

void Shader::uniform1f(Shader::LocationHandle uniform, float u) const {
    glUniform1f(getUniform(uniform), u);
}

void Shader::uniform1i(Shader::LocationHandle uniform, int u) const {
    glUniform1i(getUniform(uniform), u);
}

void Shader::uniform2f(Shader::LocationHandle uniform, float u1, float u2) const {
    glUniform2f(getUniform(uniform), u1, u2);
}

void Shader::uniform2f(Shader::LocationHandle uniform, const float* u) const {
    glUniform2fv(getUniform(uniform), 1, u);
}

void Shader::uniform2i(Shader::LocationHandle uniform, int u1, int u2) const {
    glUniform2i(getUniform(uniform), u1, u2);
}

void Shader::uniform2i(Shader::LocationHandle uniform, const int* u) const {
    glUniform2iv(getUniform(uniform), 1, u);
}

void Shader::uniform3f(Shader::LocationHandle uniform, float u1, float u2, float u3) const {
    glUniform3f(getUniform(uniform), u1, u2, u3);
}

void Shader::uniform3f(Shader::LocationHandle uniform, const float* u) const {
    glUniform3fv(getUniform(uniform), 1, u);
}

void Shader::uniform3i(Shader::LocationHandle uniform, int u1, int u2, int u3) const {
    glUniform3i(getUniform(uniform), u1, u2, u3);
}

void Shader::uniform3i(Shader::LocationHandle uniform, const int* u) const {
    glUniform3iv(getUniform(uniform), 1, u);
}

void Shader::uniform4f(Shader::LocationHandle uniform, float u1, float u2, float u3, float u4) const {
    glUniform4f(getUniform(uniform), u1, u2, u3, u4);
}

void Shader::uniform4f(Shader::LocationHandle uniform, const float* u) const {
    glUniform4fv(getUniform(uniform), 1, u);
}

void Shader::uniform4i(Shader::LocationHandle uniform, int u1, int u2, int u3, int u4) const {
    glUniform4i(getUniform(uniform), u1, u2, u3, u4);
}

void Shader::uniform4i(Shader::LocationHandle uniform, const int* u) const {
    glUniform4iv(getUniform(uniform), 1, u);
}

void Shader::uniformMatrix2f(Shader::LocationHandle uniform, const float* u) const {
    glUniformMatrix2fv(getUniform(uniform), 1, GL_FALSE, u);
}

void Shader::uniformMatrix3f(Shader::LocationHandle uniform, const float* u) const {
    glUniformMatrix3fv(getUniform(uniform), 1, GL_FALSE, u);
}

void Shader::uniformMatrix4f(Shader::LocationHandle uniform, const float* u) const {
    glUniformMatrix4fv(getUniform(uniform), 1, GL_FALSE, u);
}

void Shader::uniformVec2(Shader::LocationHandle uniform, Vec2 u) const {
    uniform2f(uniform, u.x, u.y);
}

void Shader::uniformVec3(Shader::LocationHandle uniform, Vec3 u) const {
    uniform3f(uniform, u.x, u.y, u.z);
}

void Shader::uniformVec4(Shader::LocationHandle uniform, Vec4 u) const {
    uniform4f(uniform, u.x, u.y, u.z, u.w);
}

void Shader::uniformMat4x4(Shader::LocationHandle uniform, const Mat4x4& u) const {
    uniformMatrix4f(uniform, u.matrix);
}

void Shader::attribPointer(Shader::LocationHandle attrib, int size, const void* a) const {
    attribPointer(attrib, size, 0, a);
}

void Shader::attribPointer(Shader::LocationHandle attrib, int size, int stride, const void* a) const {
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glVertexAttribPointer(getAndEnableAttrib(attrib), size, GL_FLOAT, GL_FALSE, stride, a);
}

void Shader::attribPointer(Shader::LocationHandle attrib, const VertexBuffer& a) const {
    attribPointer(attrib, 0, a);
}

void Shader::attribPointer(Shader::LocationHandle attrib, int stride, const VertexBuffer& a) const {
    a.bind(GL_ARRAY_BUFFER);
    glVertexAttribPointer(getAndEnableAttrib(attrib), a.elementSize(), GL_FLOAT, GL_FALSE, stride, a.data());
}

void Shader::uniformTexture(Shader::LocationHandle uniform, const Texture& u, GLuint slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    u.bind();
    uniform1i(uniform, slot);
}

void Shader::uniformCubeMap(Shader::LocationHandle uniform, const CubeMap& u, GLuint slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    u.bind();
    uniform1i(uniform, slot);
}
