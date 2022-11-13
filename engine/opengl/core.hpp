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