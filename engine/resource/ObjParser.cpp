#include "ObjParser.hpp"
#include "../util/log.hpp"
#include <cstring>

ObjParser::ObjParser(){}

ObjParser::~ObjParser(){
    freeCache();
}

void ObjParser::parse(std::istream& in, int n, int ni){
    freeCache();
    vertex = new float[n];
    normal = new float[n];
    index = new uint32_t[ni];
    int vpos = 0;
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
            if(buf[1] == 'n'){
                npos += parseLine(buf+2, normal+npos);
            }else if(buf[1] == ' '){
                vpos += parseLine(buf+1, vertex+vpos);
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
                index[ipos] = l;
                ipos++;
                r++;
                line = strchr(end,' ');
                if(line == NULL){
                    break;
                }
            }
            if(r != polyType){
                if(polyType == -1){
                    polyType = r;
                }else{
                    LOGW("inconsistent poly type");
                }
            }
        }
    }
    LOGD("vertex: %d/%d, normal: %d/%d, index: %d/%d",vpos,n,npos,n,ipos,ni);
    this->n = vpos;
    this->ni = ipos;
}

int ObjParser::parseLine(char *line, float *buf){
    int r = 0;
    for(;;){
        char *end;
        float f = std::strtof(line, &end);
        if(line == end){
            break;
        }
        line = end;
        *buf = f;
        buf++;
        r++;
    }
    if(r != dim){
        if(dim == -1){
            dim = r;
        }else{
            LOGW("inconsistent dimension");
        }
    }
    return r;
}

void ObjParser::freeCache(){
    if(vertex){
        delete vertex;
        vertex = NULL;
    }
    if(normal){
        delete normal;
        normal = NULL;
    }
    if(index){
        delete index;
        index = NULL;
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