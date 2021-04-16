#pragma once

#include "GL/Buffer.hpp"
#include "resource/Resource.hpp"
#include <istream>

class ObjParser {
public:
    Resource<VertexBuffer> vertex;
    Resource<VertexBuffer> texCoords;
    Resource<VertexBuffer> normals;
    Resource<IndexBuffer> indices;
    int n, vertexCount, polyType, dim;
    void parse(std::istream& in);
};
