#include "GL/Shader.hpp"
#include "util/log.hpp"
#include "util/target.hpp"

const std::string Shader::FRAGMENT_COLOR = "_fragmentColor";

ShaderBuilder::ShaderBuilder() {
    if (platform::usingGLES()) {
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
    s << fExt.str() << fInit.str() << "layout(location=0) out vec4 " << Shader::FRAGMENT_COLOR << ";\n"
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

Shader::LocationHandle ShaderBuilder::addTextureUniform(std::string type, std::string name, std::string qualifiers, std::string layout) {
    (isComputeShader ? vInit : fInit) << "layout(" << layout << (layout.empty() ? "" : ",") << "binding=" << state.textureSlot << ") " << qualifiers << "uniform " << type << " " << name << ";\n ";
    return { state.textureSlot++ };
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

void ShaderBuilder::disableFragmentShader() {
    fragmentShaderDisabled = true;
}

void ShaderBuilder::makeCompute(int_t localSizeX, int_t localSizeY, int_t localSizeZ) {
    isComputeShader = true;
    vInit << "layout(local_size_x=" << localSizeX << ", local_size_y=" << localSizeY << ", local_size_z=" << localSizeZ << ") in;\n";
}

void Shader::initialize(ShaderBuilder& builder) {
    VERIFY(GL, !glIsProgram(glId));
    glId = glCreateProgram();

    auto printSource = [](std::string source) {
        std::istringstream stream(source);
        const int bufSize = 256;
        char line[bufSize];
        int lineNumber = 1;
        while (stream.getline(line, bufSize)) {
            log<Tag::GL>("{:2d}: {}", lineNumber++, line);
        }
    };

    static constexpr const char* shaderName[] = { "Vertex", "Fragment", "Compute" };
    static constexpr GLenum shaderGLEnum[] = { GL_VERTEX_SHADER, GL_FRAGMENT_SHADER, GL_COMPUTE_SHADER };

    auto compileShader = [&](int shaderType, const char* source) {
        GLuint shader = glCreateShader(shaderGLEnum[shaderType]);
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
                shaderName[shaderType]);
            printSource(std::string(source));
            log<Tag::GL>("--------------ERROR---------------\n{}\n", message.data());
        }
        glDeleteShader(shader);
    };

    std::string vSource = builder.buildVertexSource();
    compileShader(builder.isComputeShader ? 2 : 0, vSource.c_str());

    if ((platform::usingGLES() || !builder.fragmentShaderDisabled) && !builder.isComputeShader) {
        std::string fSource = builder.buildFragmentSource();
        compileShader(1, fSource.c_str());
    }

    glLinkProgram(glId);
}
