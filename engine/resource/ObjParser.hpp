#pragma once

#include "GL/Buffer.hpp"
#include <istream>

class ObjParser {
public:
    return_ptr<VertexBuffer> vertex;
    return_ptr<VertexBuffer> texCoords;
    return_ptr<VertexBuffer> normals;
    return_ptr<IndexBuffer> indices;
    int n, vertexCount, polyType, dim;
    void parse(std::istream& in);
    void freeCache();
};
