#ifndef HEADER_RESOURCE_OBJPARSER_H
#define HEADER_RESOURCE_OBJPARSER_H

#include <istream>

class ObjParser{
    public:
        float *vertex = nullptr;
        float *texCoords = nullptr;
        float *normals = nullptr;
        uint32_t *indices = nullptr;
        int n, vertexCount, polyType, dim;
        ObjParser();
        ~ObjParser();
        void parse(std::istream& in, int nv, int ni);
        void freeCache();
};

#endif
