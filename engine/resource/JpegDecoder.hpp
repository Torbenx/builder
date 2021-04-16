#pragma once

#include "GL/Texture.hpp"
#include "resource/Resource.hpp"

class JpegDecoder {
public:
    Resource<Texture> texture;
    int width, height;

    void decode(std::istream& in);
};
