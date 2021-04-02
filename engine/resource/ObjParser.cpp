#include "resource/ObjParser.hpp"
#include "util/log.hpp"
#include <cstring>

#define MAX_POLY_TYPE 4
#define MAX_DIM 3

struct ObjParserIndex{
    int v, t, n;
};

ObjParser::ObjParser(){}

ObjParser::~ObjParser(){
    freeCache();
}

void ObjParser::parse(std::istream& in, int nv, int ni){
    freeCache();
    float *vertex = new float[MAX_DIM];
    float *normals = new float[MAX_DIM];
    ObjParserIndex *index = new ObjParserIndex[MAX_POLY_TYPE];
    float *texCoords = new float[2 * nv];
    int vpos = 0;
    int tpos = 0;
    int npos = 0;
    int ipos = 0;
    int bufsize = 256;
    char buf[bufsize];
    polyType = -1;
    dim = -1;
    while(in.getline(buf, bufsize)){
        if(in.fail()){
            LOGE("fail");
        }
        if(buf[0] == 'v'){
            if(buf[1] == ' '){
                int r = 0;
                char *line = buf+2;
                for(;;){
                    char *end;
                    float f = std::strtof(line, &end);
                    if(line == end){
                        break;
                    }
                    line = end;
                    if(++r > MAX_DIM){
                        LOGW("dimension to high: (%f, %f, %f, %f)",vertex[vpos-3],vertex[vpos-2],vertex[vpos-1],f);
                        break;
                    }
                    vertex[vpos++] = f;
                }
                if(r != dim){
                    if(dim == -1){
                        dim = r;
                        float *old = vertex;
                        vertex = new float[nv * dim];
                        std::memcpy(vertex, old, dim * sizeof(float));
                        delete[] old;
                        delete[] normals;
                        normals = new float[nv * dim];
                    }else{
                        LOGW("inconsistent dimension");
                    }
                }
            }else if(buf[1] == 't'){
                int r = 0;
                char *line = buf+2;
                for(;;){
                    char *end;
                    float f = std::strtof(line, &end);
                    if(line == end){
                        break;
                    }
                    line = end;
                    if(++r > 2){
                        if(f != 0.0f) LOGW("expected 2d texture coordinates");
                    }else{
                        texCoords[tpos++] = f;
                    }
                }
            }else if(buf[1] == 'n'){
                int r = 0;
                char *line = buf+2;
                for(;;){
                    char *end;
                    float f = std::strtof(line, &end);
                    if(line == end){
                        break;
                    }
                    line = end;
                    if(++r > MAX_DIM){
                        LOGW("dimension to high");
                        break;
                    }
                    normals[npos++] = f;
                }
                if(r != dim){
                    if(dim == -1){
                        dim = r;
                        float *old = normals;
                        normals = new float[nv * dim];
                        std::memcpy(normals, old, dim * sizeof(float));
                        delete[] old;
                        delete[] vertex;
                        vertex = new float[nv * dim];
                    }else{
                        LOGW("inconsistent dimension");
                    }
                }
            }
        }else if(buf[0] == 'f'){
            int r = 0;
            char *line = buf+1;
            for(;;){
                char *end;
                int l = std::strtol(line, &end, 10)-1;
                if(l == -1){
                    break;
                }
                if(++r > MAX_POLY_TYPE){
                    LOGW("poly type to high");
                    break;
                }
                index[ipos].v = l;
                if(end[0] == '/'){
                    line = end+1;
                    l = std::strtol(line, &end, 10)-1;
                    if(l != -1){
                        index[ipos].t = l;
                    }else{
                        LOGD("failed to parse texture index %s", line);
                        index[ipos].t = -1;
                    }
                    if(end[0] == '/'){
                        line = end+1;
                        l = std::strtol(line, &end, 10)-1;
                        if(l != -1){
                            index[ipos].n = l;
                        }else{
                            LOGD("failed to parse normal index: %s", line);
                            index[ipos].n = -1;
                        }
                    }
                }
                ipos++;
                line = end;
            }
            if(r != polyType){
                if(polyType == -1){
                    polyType = r;
                    ObjParserIndex *old = index;
                    index = new ObjParserIndex[ni * polyType];
                    std::memcpy(index, old, polyType * sizeof(uint32_t));
                    delete[] old;
                }else{
                    LOGW("inconsistent poly type");
                }
            }
        }
    }
    LOGD("vertex: %d/%d, index: %d/%d tex: %d/%d normal: %d/%d",vpos/dim,nv,ipos/polyType,ni,tpos/2,nv,npos/dim,nv);
    ni = ipos / polyType;
    int tri = polyType - 2;
    n = ni * tri * 3;
    if(false){
        this->vertexCount = n;
        this->vertex = new float[n * dim];
        this->normals = new float[n * dim];
        this->texCoords = new float[n * 2];
        for(int i=0;i<ni;i++){
            for(int j=0;j<tri;j++){
                for(int k=0;k<dim;k++){
                    this->vertex[((i*tri+j)*3  )*dim+k] = vertex[index[i*polyType    ].v*dim+k];
                    this->vertex[((i*tri+j)*3+1)*dim+k] = vertex[index[i*polyType+j+1].v*dim+k];
                    this->vertex[((i*tri+j)*3+2)*dim+k] = vertex[index[i*polyType+j+2].v*dim+k];
                    this->normals[((i*tri+j)*3  )*dim+k] = normals[index[i*polyType    ].n*dim+k];
                    this->normals[((i*tri+j)*3+1)*dim+k] = normals[index[i*polyType+j+1].n*dim+k];
                    this->normals[((i*tri+j)*3+2)*dim+k] = normals[index[i*polyType+j+2].n*dim+k];
                }
                this->texCoords[((i*tri+j)*3  )*2+0] = texCoords[index[i*polyType    ].t*2+0];
                this->texCoords[((i*tri+j)*3+1)*2+0] = texCoords[index[i*polyType+j+1].t*2+0];
                this->texCoords[((i*tri+j)*3+2)*2+0] = texCoords[index[i*polyType+j+2].t*2+0];
                this->texCoords[((i*tri+j)*3  )*2+1] = 1.0f - texCoords[index[i*polyType    ].t*2+1];
                this->texCoords[((i*tri+j)*3+1)*2+1] = 1.0f - texCoords[index[i*polyType+j+1].t*2+1];
                this->texCoords[((i*tri+j)*3+2)*2+1] = 1.0f - texCoords[index[i*polyType+j+2].t*2+1];
            }
        }
        delete[] texCoords;
        delete[] normals;
        delete[] vertex;
        delete[] index;
    }else{
        this->vertexCount = tpos / 2;
        this->indices = new uint32_t[n];
        this->vertex = new float[this->vertexCount*dim];
        this->normals = new float[this->vertexCount*dim];
        this->texCoords = texCoords;
        for(int i=1;i<tpos;i+=2){
            texCoords[i] = 1.0f - texCoords[i];
        }
        for(int i=0;i<ipos;i++){
            for(int j=0;j<dim;j++){
                this->vertex[index[i].t*dim+j] = vertex[index[i].v*dim+j];
                this->normals[index[i].t*dim+j] = normals[index[i].n*dim+j];
            }
        }
        for(int i=0;i<ni;i++){
            for(int j=0;j<tri;j++){
                this->indices[(i*tri+j)*3  ] = index[i*polyType    ].t;
                this->indices[(i*tri+j)*3+1] = index[i*polyType+j+1].t;
                this->indices[(i*tri+j)*3+2] = index[i*polyType+j+2].t;
            }
        }
        delete[] index;
        delete[] vertex;
        delete[] normals;
    }
}

void ObjParser::freeCache(){
    if(vertex){
        delete[] vertex;
        vertex = nullptr;
    }
    if(indices){
        delete[] indices;
        indices = nullptr;
    }
    if(texCoords){
        delete[] texCoords;
        texCoords = nullptr;
    }
    if(normals){
        delete[] normals;
        normals = nullptr;
    }
}
