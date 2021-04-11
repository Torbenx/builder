#pragma once

#include "GL/Texture.hpp"
#include "GL/glwrapper.hpp"

template<typename T>
class GLenumIterator {
public:
    GLenumIterator(GLenum value)
        : value(value) { }
    bool operator!=(GLenumIterator<T> other) { return value != other.value; }
    T operator*() const { return (T)value; }
    GLenumIterator<T>& operator++() {
        ++value;
        return *this;
    }

private:
    GLenum value;
};

class CubeMap : private Texture {
public:
    enum Face : GLenum {
        PosX = GL_TEXTURE_CUBE_MAP_POSITIVE_X,
        NegX = GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
        PosY = GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
        NegY = GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
        PosZ = GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
        NegZ = GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
    };

    static CubeMap color(int width, int height, Texture::Format format);
    static CubeMap depth(int width, int height, DepthBits bits);

    static inline struct {
        GLenumIterator<Face> begin() { return GLenumIterator<Face>(GL_TEXTURE_CUBE_MAP_POSITIVE_X); }
        GLenumIterator<Face> end() { return GLenumIterator<Face>(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z + 1); }
    } Faces;

    CubeMap()
        : Texture() { }
    CubeMap(const CubeMap& other)
        : Texture(other) { }

    using Texture::bind;
    using Texture::borderColor;
    using Texture::magnifyFilter;
    using Texture::minifyFilter;
    using Texture::wrappingMode;

private:
    CubeMap(Texture::Type type)
        : Texture(type, false, GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BINDING_CUBE_MAP) { }
    friend Framebuffer;
};
