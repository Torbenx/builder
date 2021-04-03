#pragma once

#include "GL/glwrapper.hpp"
#include "GL/Texture.hpp"

template< typename T >
class GLenumIterator : public std::iterator<std::input_iterator_tag, T, GLenum, const T*, T>{
public:
    GLenumIterator(GLenum value) : value(value) {}
    bool operator!=(GLenumIterator<T> other){ return value != other.value; }
    T operator*() const { return (T)value; }
    GLenumIterator<T>& operator++() { ++value; return *this; }
    GLenumIterator<T> operator++(int) { GLenumIterator<T> it = *this; value++; return it; }
private:
    GLenum value;
};

class CubeMap : private Texture{
public:
    enum Face : GLenum {
        PosX = GL_TEXTURE_CUBE_MAP_POSITIVE_X,
        NegX = GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
        PosY = GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
        NegY = GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
        PosZ = GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
        NegZ = GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
    };

    static return_ptr<CubeMap> color(int width, int height, Texture::Format format);

    static inline struct{
        GLenumIterator<Face> begin(){ return GLenumIterator<Face>(GL_TEXTURE_CUBE_MAP_POSITIVE_X); }
        GLenumIterator<Face> end(){ return GLenumIterator<Face>(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z+1); }
    } Faces;

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
