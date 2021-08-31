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
        vExt << "#version 430\n";
        fExt << "#version 430\n";
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
    vInit << "layout(location=" << state.attribIndex << ") " << qualifiers << "in " << type << " " << name << ";\n";
    return { state.attribIndex++ };
}

Shader::LocationHandle ShaderBuilder::addUniform(std::string type, std::string name, std::string qualifiers, bool vertex) {
    (vertex ? vInit : fInit) << "layout(location=" << state.uniformIndex << ") " << qualifiers << "uniform " << type << " " << name << ";\n";
    return { state.uniformIndex++ };
}

Shader::LocationHandle ShaderBuilder::beginUniformBuffer(std::string name, bool vertex) {
    (vertex ? vInit : fInit) << "layout(std140, binding=" << state.uniformBufferIndex << ") uniform " << name << "{\n";
    return { state.uniformBufferIndex++ };
}

void ShaderBuilder::endUniformBuffer(bool vertex) {
    (vertex ? vInit : fInit) << "};\n";
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
    if constexpr (Tag::GL::verifyEnabled) {
        GLint maxTextures;
        glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxTextures);
        VERIFY(GL, (GLint)state.textureSlot < maxTextures);
    }
    return state.textureSlot++;
}

void ShaderBuilder::disableFragmentShader() {
    fragmentShaderDisabled = true;
}

void Shader::initialize(ShaderBuilder& builder) {
    VERIFY(GL, !glIsProgram(glId));
    glId = glCreateProgram();
    log<Tag::GL>("initalize glId={}", glId);

    auto printSource = [](std::string source) {
        std::istringstream stream(source);
        const int bufSize = 256;
        char line[bufSize];
        int lineNumber = 1;
        while (stream.getline(line, bufSize)) {
            log<Tag::GL>("{:2d}: {}", lineNumber++, line);
        }
    };

    auto compileShader = [&](bool vertex, const char* source) {
        GLuint shader = glCreateShader(vertex ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER);
        glShaderSource(shader, 1, &source, 0);
        glCompileShader(shader);
        GLint success = GL_FALSE;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (success == GL_TRUE) {
            glAttachShader(glId, shader);
        } else if(Tag::GL::verifyEnabled) {
            GLint length = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
            std::vector<char> message(length);
            glGetShaderInfoLog(shader, length, &length, message.data());
            log<Tag::GL>("\n"
                 "{} Shader failed to compile:\n"
                 "--------------SOURCE--------------\n",
                vertex ? "Vertex" : "Fragment");
            printSource(std::string(source));
            log<Tag::GL>("--------------ERROR---------------\n{}\n", message.data());
        }
        printSource(std::string(source));
        glDeleteShader(shader);
    };

    std::string vSource = builder.buildVertexSource();
    compileShader(true, vSource.c_str());

    if (usingGLES() || !builder.fragmentShaderDisabled) {
        std::string fSource = builder.buildFragmentSource();
        compileShader(false, fSource.c_str());
    }

    glLinkProgram(glId);
}
