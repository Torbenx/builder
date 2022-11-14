#pragma once

#include "graphics/Buffer.hpp"
#include "graphics/shadergen/Shader.hpp"
#include "platform/Window.hpp"
#include "software/Semaphore.hpp"
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

inline GLenum toGL(shadergen::Binding::Access access) {
    static constexpr GLenum glValues[] = {
        GL_READ_ONLY,
        GL_READ_ONLY,
        GL_WRITE_ONLY,
        GL_READ_WRITE,
    };
    return glValues[stx::to_underlying(access)];
}

inline GLenum toGL(gfx::TexelComponents format) {
    using enum gfx::TexelComponents;
    switch (format){
    case RGBA8:
        return GL_RGBA8;
    default:
        VERIFY_NOT_REACHED(OpenGL);
        return GL_NONE;
    }
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

struct ImageType {
    static constinit const ImageType TYPES[8];
    GLenum bindpoint;
    GLenum target;
    int layer;
    static ImageType get(gfx::Image<> image) { return get(image.type); }
    static ImageType get(gfx::ImageType type) { return TYPES[stx::to_underlying(type)]; }
};
inline constinit const ImageType ImageType::TYPES[8] = {
    { GL_TEXTURE_2D, GL_TEXTURE_2D, 0 },
    { GL_RENDERBUFFER, GL_RENDERBUFFER, 0 },
    { GL_TEXTURE_CUBE_MAP, GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0 },
    { GL_TEXTURE_CUBE_MAP, GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 1 },
    { GL_TEXTURE_CUBE_MAP, GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 2 },
    { GL_TEXTURE_CUBE_MAP, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 3 },
    { GL_TEXTURE_CUBE_MAP, GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 4 },
    { GL_TEXTURE_CUBE_MAP, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 5 },
};

inline void bind(const shadergen::ShaderRecord* shader, const void* inData) {
    const std::byte* data = (const std::byte*)inData;
    gfx::Frame* frame = gfx::frame();
    using BType = shadergen::Binding::Type;
    for (const auto& binding : shader->bindings) {
        if (binding.type == BType::StorageBuffer || binding.type == BType::UniformBuffer) {
            auto gfxHandle = *(const gfx::BufferHandle<>*)(data + binding.offset);
            auto glBuf = frame->hwBuffers[gfxHandle.id];
            GLenum glTarget = binding.type == BType::UniformBuffer ? GL_UNIFORM_BUFFER : GL_SHADER_STORAGE_BUFFER;
            glBindBufferRange(glTarget, binding.index, glBuf.handle, glBuf.offset, glBuf.size);
        } else if (binding.type == BType::StorageImage) {
            auto gfxHandle = *(const gfx::Image<>*)(data + binding.offset);
            auto glType = ImageType::get(gfxHandle);
            auto glBaseTexture = frame->hwObjects[gfxHandle.baseId];
            glBindImageTexture(binding.index, glBaseTexture.handle, gfxHandle.mipLevel, false, glType.layer, toGL(binding.access), toGL(binding.imageFormat));
        } else {
            VERIFY_NOT_REACHED(OpenGL);
        }
    }
}

/*inline void imageData(Image image, ui::IntRect rect, sw::TexturePosition data) {
    image.bindTexture();
    glPixelStorei(GL_UNPACK_ROW_LENGTH, data.stride);
    glTexSubImage2D(image.glTarget(), image.mipLevel,
        rect.left(), rect.bottom(), rect.width(), rect.height(),
        GL_RGBA, GL_UNSIGNED_BYTE, data.data);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
}
inline void imageData(Image image, sw::IntVec offset, sw::TextureView data) {
    imageData(image, ui::IntRect { ui::BottomLeft(offset), ui::Dimensions(data.dimensions) }, data.pos());
}*/

}