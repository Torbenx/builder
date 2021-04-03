#include "GL/CubeMap.hpp"

return_ptr<CubeMap> CubeMap::color(int width, int height, Texture::Format format){
    CubeMap *cubeMap = new CubeMap(Color);
    cubeMap->bind();
    for(Face face : Faces){
        glTexImage2D(face, 0, Texture::glFormatOf(format), width, height, 0, Texture::glBaseFormatOf(format), Texture::glTypeOf(format), 0);
    }
    cubeMap->minifyFilter(Linear);
    return cubeMap;
}

CubeMap::CubeMap(CubeMap&& other) : Texture(std::move(other)) {}

CubeMap::CubeMap(Texture::Type type) : Texture(type, false, GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BINDING_CUBE_MAP) {}
