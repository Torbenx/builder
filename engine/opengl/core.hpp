#pragma once

#include "containers/EnumTable.hpp"
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
    switch (format) {
    case RGBA8:
        return GL_RGBA8;
    default:
        VERIFY_NOT_REACHED(OpenGL);
        return GL_NONE;
    }
}

struct BufferHandle : gfx::HWObject {
    using gfx::HWObject::HWObject;
};

inline std::pair<BufferHandle, void*> allocateAndMapBuffer(int_t size, GLbitfield flags) {
    static constexpr GLenum TEMP_TARGET = GL_COPY_READ_BUFFER;
    flags |= GL_MAP_COHERENT_BIT | GL_MAP_PERSISTENT_BIT;
    GLuint id = 0;
    glGenBuffers(1, &id);
    glBindBuffer(TEMP_TARGET, id);
    glBufferStorage(TEMP_TARGET, size, nullptr, flags);
    void* mapping = glMapBufferRange(TEMP_TARGET, 0, size, flags);
    return { id, mapping };
}
inline std::pair<BufferHandle, void*> allocateReadBuffer(int_t size) {
    return allocateAndMapBuffer(size, GL_MAP_READ_BIT);
}
inline std::pair<BufferHandle, void*> allocateWriteBuffer(int_t size) {
    return allocateAndMapBuffer(size, GL_MAP_WRITE_BIT);
}

struct ImageType {
    static constinit const stx::EnumTable<gfx::ImageType, ImageType> TYPES;
    GLenum bindpoint = GL_NONE;
    GLenum target = GL_NONE;
    int layer = 0;
    static ImageType get(gfx::Image<> image) { return get(image.type); }
    static ImageType get(gfx::ImageType type) { return TYPES[type]; }
};
inline constinit const stx::EnumTable<gfx::ImageType, ImageType> ImageType::TYPES {
    {},
    {
        { gfx::ImageType::Texture, { GL_TEXTURE_2D, GL_TEXTURE_2D, 0 } },
        { gfx::ImageType::CubeFacePosX, { GL_TEXTURE_CUBE_MAP, GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0 } },
        { gfx::ImageType::CubeFaceNegX, { GL_TEXTURE_CUBE_MAP, GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 1 } },
        { gfx::ImageType::CubeFacePosY, { GL_TEXTURE_CUBE_MAP, GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 2 } },
        { gfx::ImageType::CubeFaceNegY, { GL_TEXTURE_CUBE_MAP, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 3 } },
        { gfx::ImageType::CubeFacePosZ, { GL_TEXTURE_CUBE_MAP, GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 4 } },
        { gfx::ImageType::CubeFaceNegZ, { GL_TEXTURE_CUBE_MAP, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 5 } },
    },
};

inline void bind(gfx::FrameData* frame, const shadergen::ShaderRecord* shader, const void* inData) {
    const std::byte* data = (const std::byte*)inData;
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