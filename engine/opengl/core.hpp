#pragma once

#include "graphics/shadergen/Shader.hpp"
#include "platform/Window.hpp"
#include "software/Semaphore.hpp"
#include "software/Texture.hpp"
#include "util/Callable.hpp"
#include "util/target.hpp"

namespace Tag {
struct OpenGL {
    static constexpr char tag[] = "gl";
    static constexpr bool verifyEnabled = true;
};
}

#ifdef PLATFORM_WINDOWS
#include "platform/windows/glcore.hpp"
#endif

namespace gl {

void initialize();
void attachWindow(plt::Window* window);

static GLenum toGL(shadergen::Binding::Access access) {
    static constexpr GLenum glValues[] = {
        GL_READ_ONLY,
        GL_READ_ONLY,
        GL_WRITE_ONLY,
        GL_READ_WRITE,
    };
    return glValues[stx::to_underlying(access)];
}

struct CommandSlot {
    stx::BasicCallback<>* slot = nullptr;
    void set(stx::BasicCallback<> callback) const {
        *slot = callback;
    }
    explicit operator bool() const { return (bool)slot; }
};

struct Image {
    enum class Bindpoint : uint32_t {
        Texture = 0,
        CubeMap = 1,
        Renderbuffer = 2,
    };
    enum class Target : uint32_t {
        Texture = 0,
        Renderbuffer = 1,
        FacePosX = 2,
        FaceNegX = 3,
        FacePosY = 4,
        FaceNegY = 5,
        FacePosZ = 6,
        FaceNegZ = 7,
    };
    static GLenum toGL(Bindpoint bindpoint) {
        static constexpr GLenum glValues[] = {
            GL_TEXTURE_2D,
            GL_TEXTURE_CUBE_MAP,
            GL_RENDERBUFFER,
        };
        return glValues[stx::to_underlying(bindpoint)];
    }
    static GLenum toGL(Target target) {
        static constexpr GLenum glValues[] = {
            GL_TEXTURE_2D,
            GL_RENDERBUFFER,
            GL_TEXTURE_CUBE_MAP_POSITIVE_X,
            GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
            GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
            GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
            GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
            GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
        };
        return glValues[stx::to_underlying(target)];
    }
    uint32_t glId = 0;
    Bindpoint bindpoint : 8 = (Bindpoint)0;
    Target target : 8 = (Target)0;
    uint32_t mipLevel : 16 = 0;
    explicit operator bool() const { return glId != 0; }

    GLenum glBindpoint() const { return toGL(bindpoint); }
    GLenum glTarget() const { return toGL(target); }

    void bindTexture() const {
        glBindTexture(toGL(bindpoint), glId);
    }
};

class Buffer {
public:
    static Buffer makeStatic(int_t size) { return make(nullptr, size, 0); }
    static Buffer makeWriteOnly(int_t size) { return make(nullptr, size, GL_MAP_WRITE_BIT); }
    static Buffer makeReadOnly(int_t size) { return make(nullptr, size, GL_MAP_READ_BIT); }
    static Buffer makeReadWrite(int_t size) { return make(nullptr, size, GL_MAP_WRITE_BIT | GL_MAP_READ_BIT); }
    static Buffer make(int_t size, bool read, bool write) { return make(nullptr, size, (read ? GL_MAP_READ_BIT : 0) | (write ? GL_MAP_WRITE_BIT : 0)); }
    static Buffer makeStatic(void* data, int_t size) { return make(data, size, 0); }
    static Buffer makeWriteOnly(void* data, int_t size) { return make(data, size, GL_MAP_WRITE_BIT); }
    static Buffer makeReadOnly(void* data, int_t size) { return make(data, size, GL_MAP_READ_BIT); }
    static Buffer makeReadWrite(void* data, int_t size) { return make(data, size, GL_MAP_WRITE_BIT | GL_MAP_READ_BIT); }
    static Buffer make(void* data, int_t size, bool read, bool write) { return make(data, size, (read ? GL_MAP_READ_BIT : 0) | (write ? GL_MAP_WRITE_BIT : 0)); }

    void* mapReadable(int_t offset, int_t size) const { return map(offset, size, GL_MAP_READ_BIT); }
    void* mapWritable(int_t offset, int_t size) const { return map(offset, size, GL_MAP_WRITE_BIT); }
    void* mapReadWritable(int_t offset, int_t size) const { return map(offset, size, GL_MAP_READ_BIT | GL_MAP_WRITE_BIT); }
    void* map(int_t offset, int_t size, bool read, bool write) const { return map(offset, size, (read ? GL_MAP_READ_BIT : 0) | (write ? GL_MAP_WRITE_BIT : 0)); }

    void unmap() const {
        glBindBuffer(TEMP_BUFFER_TARGET, glId);
        glUnmapBuffer(TEMP_BUFFER_TARGET);
    }

    void bind(GLenum target) const {
        glBindBuffer(target, glId);
    }

    void deallocate() {
        if (glId != 0)
            glDeleteBuffers(1, &glId);
        glId = 0;
    }

    uint32_t glId = 0;

private:
    static constexpr GLenum TEMP_BUFFER_TARGET = GL_COPY_WRITE_BUFFER;

    static Buffer make(void* data, int_t size, GLbitfield flags) {
        if (flags != 0) {
            flags |= GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
        }
        Buffer ret;
        glGenBuffers(1, &ret.glId);
        glBindBuffer(TEMP_BUFFER_TARGET, ret.glId);
        glBufferStorage(TEMP_BUFFER_TARGET, size, data, flags);
        return ret;
    }

    void* map(int_t offset, int_t size, GLbitfield flags) const {
        glBindBuffer(TEMP_BUFFER_TARGET, glId);
        return glMapBufferRange(TEMP_BUFFER_TARGET, offset, size, flags | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
    }
};

struct Texture {
    uint32_t glId = 0;
    explicit operator bool() const { return glId != 0; }
    Image image(int_t mipLevel = 0) const {
        return {
            .glId = glId,
            .bindpoint = Image::Bindpoint::Texture,
            .target = Image::Target::Texture,
            .mipLevel = (uint32_t)mipLevel,
        };
    }
    void bind() const { glBindTexture(GL_TEXTURE_2D, glId); }
    operator Image() const { return image(); }

    void deallocate() {
        if (glId != 0)
            glDeleteTextures(1, &glId);
        glId = 0;
    }
};

struct CubeMap {
    enum class Face {
        PosX = 0,
        NegX = 1,
        PosY = 2,
        NegY = 3,
        PosZ = 4,
        NegZ = 5,
    };
    uint32_t glId = 0;
    explicit operator bool() const { return glId != 0; }
    Image face(Face face, int_t mipLevel = 0) {
        return {
            .glId = glId,
            .bindpoint = Image::Bindpoint::CubeMap,
            .target = (Image::Target)(stx::to_underlying(Image::Target::FacePosX) + stx::to_underlying(face)),
            .mipLevel = (uint32_t)mipLevel,
        };
    }

    void deallocate() {
        if (glId != 0)
            glDeleteTextures(1, &glId);
        glId = 0;
    }
};

inline void bind(shadergen::Binding binding, Image image) {
    VERIFY(OpenGL, binding.type == shadergen::Binding::Type::StorageImage);
    int layer = 0;
    if (image.bindpoint == Image::Bindpoint::CubeMap)
        layer = stx::to_underlying(image.target) - stx::to_underlying(Image::Target::FacePosX);
    glBindImageTexture(binding.index, image.glId, image.mipLevel, false, layer, toGL(binding.access), GL_RGBA8UI);
}
inline void bind(shadergen::Binding binding, Texture texture) {
    VERIFY(OpenGL, binding.type == shadergen::Binding::Type::SampledImage);
    glActiveTexture(GL_TEXTURE0 + binding.index);
    texture.bind();
}
inline void bind(shadergen::Binding binding, Buffer buffer, int_t offset, int_t size) {
    using enum shadergen::Binding::Type;
    VERIFY(OpenGL, binding.type == UniformBuffer || binding.type == StorageBuffer);
    GLenum target = binding.type == UniformBuffer ? GL_UNIFORM_BUFFER : GL_SHADER_STORAGE_BUFFER;
    glBindBufferRange(target, binding.index, buffer.glId, offset, size);
}
template<const shadergen::Shader& shader, stx::FixedString name>
constexpr shadergen::Binding binding = shadergen::getBinding<shader, name>;

inline void imageData(Image image, ui::IntRect rect, sw::TexturePosition data) {
    image.bindTexture();
    glPixelStorei(GL_UNPACK_ROW_LENGTH, data.stride);
    glTexSubImage2D(image.glTarget(), image.mipLevel,
        rect.left(), rect.bottom(), rect.width(), rect.height(),
        GL_RGBA, GL_UNSIGNED_BYTE, data.data);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
}
inline void imageData(Image image, sw::IntVec offset, sw::TextureView data) {
    imageData(image, ui::IntRect { ui::BottomLeft(offset), ui::Dimensions(data.dimensions) }, data.pos());
}

}