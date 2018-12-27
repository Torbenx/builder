#include "ObjParser.hpp"
#include "../util/log.hpp"
#include <cstring>

#define MAX_POLY_TYPE 4
#define MAX_DIM 3

ObjParser::ObjParser(){}

ObjParser::~ObjParser(){
    freeCache();
}

void ObjParser::parse(std::istream& in, int nv, int ni){
    freeCache();
    float *vertex = new float[MAX_DIM];
    uint32_t *index = new uint32_t[MAX_POLY_TYPE];
    int vpos = 0;
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
                char *line = buf+1;
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
                if(line == end){
                    break;
                }
                if(++r > MAX_POLY_TYPE){
                    LOGW("poly type to high");
                    break;
                }
                index[ipos++] = l;
                line = strchr(end,' ');
                if(line == nullptr){
                    break;
                }
            }
            if(r != polyType){
                if(polyType == -1){
                    polyType = r;
                    uint32_t *old = index;
                    index = new uint32_t[ni * polyType];
                    std::memcpy(index, old, polyType * sizeof(uint32_t));
                    delete[] old;
                }else{
                    LOGW("inconsistent poly type");
                }
            }
        }
    }
    LOGD("vertex: %d/%d, index: %d/%d",vpos/dim,nv,ipos/polyType,ni);
    nv = vpos / dim;
    ni = ipos / polyType;
    int tri = polyType - 2;
    n = ni * tri * 3;
    this->vertexSize = n * dim;
    this->vertex = new float[this->vertexSize];
    for(int i=0;i<ni;i++){
        for(int j=0;j<tri;j++){
            for(int k=0;k<dim;k++){
                this->vertex[((i*tri+j)*3  )*dim+k] = vertex[index[i*polyType    ]*dim+k];
                this->vertex[((i*tri+j)*3+1)*dim+k] = vertex[index[i*polyType+j+1]*dim+k];
                this->vertex[((i*tri+j)*3+2)*dim+k] = vertex[index[i*polyType+j+2]*dim+k];
            }
        }
    }
    delete[] vertex;
    delete[] index;
}

void ObjParser::freeCache(){
    if(vertex){
        delete[] vertex;
        vertex = nullptr;
    }
}

/*void readObj(char *filename,float *vertex,float *normal,int *indices){
    FILE *fp = fopen(filename,"r");
    size_t read;
    size_t len = 0;
    size_t normalPos = 0;
    size_t vertexPos = 0;
    size_t indicesPos = 0;
    char *line;
    float x,y,z;
    int index1,index2;
    while((read=getLine(&line,&len,fp))!=-1){
        len+=read;
        if(*line=="v"){
            if(*(line+1)==" "){
                readObjCoordLine(line,vertex+vertexPos);
                vertexPos += 3;
            }else if(*(line+1)=="n"){
                readObjCoordLine(line,normal+normalPos);
                normalPos += 3;
            }else{
                //ignore
            }
        }else if(*line=="f"){
            int x,y,z,index1,index2;
            index1 = indexOf(line+3,"/");
            x = strtol(line+3,line+index1,0);
            index2 = indexOf(line+index1," ");
            index1 = indexOf(line+index2,"/");
            y = strtol(line+index2+1,line+index1,0);
            index2 = indexOf(line+index1," ");
            index1 = indexOf(line+index2,"/");
            z = strtol(line+index2+1,line+index1,0);
            *(indices+(indicesPos++)) = x;
            *(indices+(indicesPos++)) = y;
            *(indices+(indecesPos++)) = z;
        }else{
            //ignore
        }
    }
}

int indexOf(char *string,char *search){
    int i;
    while(1){
        if(string+i==search){
            return i;
        }else if(string+i=="\0"){
            return -1;
        }else{
            i++;
        }
    }
}

void readObjCoordLine(char *line,float *coords){
    int x,y,z,index1,index2;
    index1 = indexOf(line+3," ");
    x = strtof(line+3,line+index1);
    index2 = indexOf(line+index1+1," ");
    y = strtof(line+index1+1,line+index2);
    z = strtof(line+index2+1,line+read);
    *(coords) = x;
    *(coords+1) = y;
    *(coords+2) = z;
}*/