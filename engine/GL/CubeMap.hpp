#pragma once

#include "GL/glwrapper.hpp"
#include "GL/Texture.hpp"

class CubeMap : private Texture{
public:
    enum Face : GLenum {
        PosX = GL_TEXTURE_CUBE_MAP_POSITIVE_X,
        NegX = GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
        PosY = GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
        NegY = GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
        PosZ = GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
        NegZ = GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
    };

    static return_ptr<CubeMap> color(int width, int height, Texture::Format format);

    CubeMap(CubeMap&&);

    using Texture::bind;
    using Texture::wrappingMode;
    using Texture::magnifyFilter;
    using Texture::minifyFilter;
    using Texture::borderColor;

private:
    CubeMap(Texture::Type);
    CubeMap(const CubeMap& other);
    friend Framebuffer;
};
