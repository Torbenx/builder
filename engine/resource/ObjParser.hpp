#ifndef HEADER_RESOURCE_OBJPARSER_H
#define HEADER_RESOURCE_OBJPARSER_H

#include <istream>
#include "../util/SourceGetter.hpp"

class ObjParser{
    public:
        float *vertex = nullptr;
        int n, vertexSize /*in floats*/, polyType, dim;
        ObjParser();
        ~ObjParser();
        void parse(std::istream& in, int nv, int ni);
        SourceGetter<float*>* getVertexSource();
        SourceGetter<float*>* getNormalSource();
        void freeCache();
};

#endif