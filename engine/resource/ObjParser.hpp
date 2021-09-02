#pragma once

#include "GL/Buffer.hpp"
#include "math/Box.hpp"
#include "resource/Resource.hpp"
#include <istream>

class ObjParser {
public:
    float* vertex = nullptr;
    float* texCoords = nullptr;
    float* normals = nullptr;
    GLuint* indices = nullptr;
    Box boundingBox;
    int n, vertexCount, polyType, dim;
    void parse(std::istream& in);
    void clear() {
        if (vertex) {
            delete[] vertex;
            vertex = nullptr;
        }
        if (texCoords) {
            delete[] texCoords;
            texCoords = nullptr;
        }
        if (normals) {
            delete[] normals;
            normals = nullptr;
        }
        if (indices) {
            delete[] indices;
            indices = nullptr;
        }
    }
    ~ObjParser() { clear(); }
};
