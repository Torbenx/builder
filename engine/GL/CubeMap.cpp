#include "GL/CubeMap.hpp"
#include "util/log.hpp"

CubeMap CubeMap::color(int width, int height, Texture::Format format) {
    CubeMap cubeMap(Texture::Type::Color);
    cubeMap.bind();
    for (Face face : Faces) {
        glTexImage2D(face, 0, Texture::glFormatOf(format), width, height, 0, Texture::glBaseFormatOf(format), Texture::glTypeOf(format), 0);
    }
    cubeMap.minifyFilter(Linear);
    return cubeMap;
}

CubeMap CubeMap::depth(int width, int height, Texture::DepthBits bits) {
    CubeMap cubeMap(Texture::Type::Depth);
    cubeMap.bind();
    for (Face face : Faces) {
        glTexImage2D(face, 0, bits, width, height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, 0);
    }
    cubeMap.minifyFilter(Linear);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    return cubeMap;
}
